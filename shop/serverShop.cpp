#include "httplib.h"
#include <nlohmann/json.hpp>
#include <cppconn/driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <iostream>
#include "loginServer.h"
#include "signUpServer.h"

using namespace httplib;
using namespace sql;
using json = nlohmann::json;

Connection *con;

json allProducts() {
    json products = json::array();
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM product");
        ResultSet* rSet = pstmt->executeQuery();
        while(rSet->next()) {
            products.push_back({
                {"id", rSet->getInt("id")},
                {"name", rSet->getString("name")},
                {"price", rSet->getInt("price")},
                {"quantity", rSet->getInt("quantity")},
                {"color", rSet->getString("color")}
            });
        }
        delete rSet;
        delete pstmt;
    } catch(sql::SQLException &e) {
        products["error"] = e.what();
    }
    return products;
}

bool createUserCart(int userId) {
    try {
        string tableName = "user_" + std::to_string(userId) + "_table";
        PreparedStatement* check = con->prepareStatement("SELECT had_cart FROM users WHERE id = ?");
        check->setInt(1, userId);
        ResultSet* rSet = check->executeQuery();
        bool hadCart = false;
        if(rSet->next()) hadCart = rSet->getBoolean("had_cart");
        delete rSet;
        delete check;
        if(hadCart) return false;

        string query = "CREATE TABLE " + tableName + " ("
                       "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
                       "product_id INT NOT NULL, "
                       "quantity INT NOT NULL, "
                       "color VARCHAR(50), "
                       "price INT, "
                       "final_price DOUBLE, "
                       "product_name VARCHAR(50)"
                       ")";
        Statement* stmt = con->createStatement();
        stmt->execute(query);
        delete stmt;

        PreparedStatement* update = con->prepareStatement("UPDATE users SET had_cart = true WHERE id = ?");
        update->setInt(1,userId);
        update->executeUpdate();
        delete update;
        return true;
    } catch(sql::SQLException &e) {
        return false;
    }
}

json addToCartAPI(int userId, int productId, int quantity) {
    json result;
    string tableName = "user_" + std::to_string(userId) + "_table";
    try {
        createUserCart(userId);
        PreparedStatement* pstmt = con->prepareStatement("SELECT name, color, price, quantity FROM product WHERE id=?");
        pstmt->setInt(1, productId);
        ResultSet* rSet = pstmt->executeQuery();
        if(!rSet->next()) {
            delete rSet; delete pstmt;
            result["error"] = "Product not found";
            return result;
        }

        int available = rSet->getInt("quantity");
        if(quantity > available) {
            result["error"] = "Not enough stock";
            delete rSet; delete pstmt;
            return result;
        }

        string productName = rSet->getString("name");
        string color = rSet->getString("color");
        double price = rSet->getDouble("price");
        delete rSet; delete pstmt;

        string insertSQL = "INSERT INTO " + tableName + " (product_id, quantity, color, price, final_price, product_name) VALUES (?,?,?,?,?,?)";
        PreparedStatement* addItem = con->prepareStatement(insertSQL);
        addItem->setInt(1, productId);
        addItem->setInt(2, quantity);
        addItem->setString(3, color);
        addItem->setDouble(4, price);
        addItem->setDouble(5, price * quantity);
        addItem->setString(6, productName);
        addItem->executeUpdate();
        delete addItem;

        PreparedStatement* updateStock = con->prepareStatement("UPDATE product SET quantity = quantity - ? WHERE id=?");
        updateStock->setInt(1, quantity);
        updateStock->setInt(2, productId);
        updateStock->executeUpdate();
        delete updateStock;

        result["status"] = "ok";
        result["message"] = "Added to cart";
    } catch(sql::SQLException &e) {
        result["error"] = e.what();
    }
    return result;
}

json viewCart(int userId) {
    json cart = json::array();
    string tableName = "user_" + std::to_string(userId) + "_table";
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM " + tableName);
        ResultSet* rSet = pstmt->executeQuery();
        while(rSet->next()) {
            cart.push_back({
                {"id", rSet->getInt("id")},
                {"product_id", rSet->getInt("product_id")},
                {"quantity", rSet->getInt("quantity")},
                {"name", rSet->getString("product_name")},
                {"price", rSet->getDouble("price")},
                {"final_price", rSet->getDouble("final_price")},
                {"color", rSet->getString("color")}
            });
        }
        delete rSet; delete pstmt;
    } catch(sql::SQLException &e) {
        cart["error"] = e.what();
    }
    return cart;
}

json getValetBalance(int userId) {
    json result;
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT balance FROM valet WHERE user_id=?");
        pstmt->setInt(1,userId);
        ResultSet* rSet = pstmt->executeQuery();
        if(rSet->next()) {
            result["balance"] = rSet->getInt("balance");
        } else result["balance"] = 0;
        delete rSet; delete pstmt;
    } catch(sql::SQLException &e) {
        result["error"] = e.what();
    }
    return result;
}

int main() {
    Driver* driver = get_driver_instance();
    con = driver->connect("tcp://127.0.0.1:3306", "root", "MyNewPassword123");
    con->setSchema("shop");

    Server svr;


    svr.Post("/signup", [&](const httplib::Request& req, httplib::Response& res) {
        auto username = req.get_param_value("username");
        auto password = req.get_param_value("password");
        auto confirm  = req.get_param_value("confirm");
        auto email    = req.get_param_value("email");

        json j = signUpHttp(con, username, password, confirm, email);
        res.set_content(j.dump(), "application/json");
    });

    svr.Post("/login", [&](const httplib::Request& req, httplib::Response& res) {
        auto username = req.get_param_value("username");
        auto password = req.get_param_value("password");

        json j = loginHttp(con, username, password);
        res.set_content(j.dump(), "application/json");
    });



    svr.Get("/products", [](const Request &req, Response &res){
        res.set_content(allProducts().dump(), "application/json");
    });

    svr.Post(R"(/addtocart/(\d+))", [](const Request &req, Response &res){
        int userId = stoi(req.matches[1]);
        json body = json::parse(req.body);
        int productId = body["product_id"];
        int quantity = body["quantity"];
        res.set_content(addToCartAPI(userId, productId, quantity).dump(), "application/json");
    });

    svr.Get(R"(/cart/(\d+))", [](const Request &req, Response &res){
        int userId = stoi(req.matches[1]);
        res.set_content(viewCart(userId).dump(), "application/json");
    });

    svr.Get(R"(/valet/(\d+))", [](const Request &req, Response &res){
        int userId = stoi(req.matches[1]);
        res.set_content(getValetBalance(userId).dump(), "application/json");
    });

    std::cout << "Server running at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);

    delete con;
    return 0;
}
