#include <iostream>
#include <jwt-cpp/jwt.h>
#include <nlohmann/json.hpp>
#include "httplib.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <ctime>
#include "BCrypt.hpp"
#include <iomanip>
#include "login.h"
#include <vector>
using namespace sql;
using namespace std;
using namespace httplib;
using json = nlohmann::json;

Connection *con;
int sumOfProducts = 0;
void sumProduct(int &value)
{
    value = 0;
    PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM product");
    ResultSet* rSet = pstmt->executeQuery();
    while(rSet->next()) {
        int mainQuantity = rSet->getInt("quantity");            
        value += 1;
    }
    delete rSet;
    delete pstmt;

}
void deleteZeroProductQuantity() {
    try {
        PreparedStatement* deleteProduct = con->prepareStatement("DELETE FROM product WHERE quantity = 0");
        deleteProduct->executeUpdate();
        delete deleteProduct;
    } catch(sql::SQLException &e) {
        cerr << "SQL Error (deleteZeroProductQuantity): " << e.what() << endl;
    }
}
void deleteZeroProductQuantityFromCard(const string& tableName) {
    try {
        string sql = "DELETE FROM " + tableName + " WHERE quantity = 0";
        PreparedStatement* deleteProduct = con->prepareStatement(sql);
        deleteProduct->executeUpdate();
        delete deleteProduct;
    } catch(sql::SQLException &e) {
        cerr << "SQL Error (deleteZeroProductQuantity): " << e.what() << endl;
    }
}
void allProduct()
{ 
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM product");
        ResultSet* rSet = pstmt->executeQuery();
        cout << left << setw(5)  << "ID" << setw(20) << "Name" << setw(14) << "Price" << setw(12) << "Quantity" << setw(10) << "Color" << endl;
        
        cout << string(60, '-') << endl;
        
        while(rSet->next()) {
            cout << left << setw(5)  << rSet->getInt("id") << setw(20) << rSet->getString("name") << setw(15) << fixed << rSet->getInt("price") << setw(12) << rSet->getInt("quantity") << setw(10) << rSet->getString("color") << endl;
        }

        delete rSet;
        delete pstmt;
        
    } catch(sql::SQLException &e) {
        cerr << "SQL Error (allProduct): " << e.what() << endl;
    }
}
void creatCard(const string& tableName,int id)
{
    
    try {
        bool hadCart = true;
        PreparedStatement *finalT = con->prepareStatement("SELECT had_cart FROM users WHERE id = ?");
        finalT->setInt(1,id);
        ResultSet *rSet = finalT->executeQuery();
        if(rSet->next()) {
            hadCart = rSet->getBoolean("had_cart");
        } else {
            cout << "User not found!" << endl;
        }

        delete rSet;
        delete finalT;
        if (hadCart == 1)
        {
            return;
        }
        
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
        
        cout << "Table " << tableName << " created successfully." << endl;
        
    } catch (SQLException &e) {
        cerr << "error createCart: " << e.what() << endl;
    }
}
void addToCard(const string& tableName)
{
    int userId = 1;
    int productId,quantity;
    bool flag = true;
    while (true) {
        cout << endl << "Enter Product ID : ";
        cin >> productId;

        if (!cin.fail()) {
            break;
        }

        cout << "enter valid product id" << endl;
        cin.clear();
        cin.ignore(1000, '\n');
    }
    
    
    try {
        PreparedStatement* pstmt = con->prepareStatement( "SELECT quantity,color, price, name FROM product WHERE id = ?");
        pstmt->setInt(1, productId);
        ResultSet* rSet = pstmt->executeQuery();
        if (!rSet->next()) {
            cout << "Product not found with this id!" << endl;
            delete rSet;
            delete pstmt;
            return;
        }
        while (true) {
            cout << "Enter quantity : ";
            cin >> quantity;
            
            if (!cin.fail()) {
                break;
            }
        
            cout << "enter valid quantity" << endl;
            cin.clear();
            cin.ignore(1000, '\n');
            }
        int available = rSet->getInt("quantity");
        
        if (available < quantity) {
            cout << "Not enough available : " << available << endl;
            delete rSet;
            delete pstmt;
            return;
        }
        
        string productName = rSet->getString("name");
        string productColor = rSet->getString("color");
        double price = rSet->getDouble("price");
        
        delete rSet;
        delete pstmt;
        
        string sql = "INSERT INTO " + tableName + " (product_id, quantity, color, price, final_price, product_name) VALUES (?,?,?,?,?,?)";
        PreparedStatement* addItem = con->prepareStatement(sql);

        addItem->setInt(1, productId);
        addItem->setInt(2, quantity);   
        addItem->setString(3, productColor);
        addItem->setDouble(4, price);
        addItem->setDouble(5, price * quantity);
        addItem->setString(6, productName);
        addItem->executeUpdate();
        
        delete addItem;
        
        PreparedStatement* updateStmt = con->prepareStatement("UPDATE product SET quantity = quantity - ? WHERE id = ?");
        updateStmt->setInt(1, quantity);
        updateStmt->setInt(2, productId);
        updateStmt->executeUpdate();
        delete updateStmt;
        
        cout << endl << "product added to cart successfully" << endl;
        
        
    } catch (sql::SQLException &e) {
        cerr << "error addToCart : " << e.what() << endl;
    }
}
void allCartItems(const string& tableName)
{
    deleteZeroProductQuantityFromCard(tableName);
    char choice = 'y';
    cout << "Your all cart items:" << endl;

    string show = "SELECT id, quantity, color, price, final_price, product_name AS name FROM " + tableName;
    PreparedStatement* showAll = con->prepareStatement(show);
    ResultSet *rSet = showAll->executeQuery();

    cout << left << setw(10) << "ID" << setw(20) << "Name" << setw(15) << "Price" << setw(12) << "Quantity" << setw(10) << "Color" << setw(10) << "final price" << endl;
    cout << string(70, '-') << endl;

    while(rSet->next()) {
        cout << left << setw(10) << rSet->getInt("id") << setw(20) << rSet->getString("name") << setw(15) << fixed << rSet->getInt("price") << setw(12) << rSet->getInt("quantity") << setw(10) << rSet->getString("color") << setw(10) << rSet->getInt("final_price") << endl;
    }

    delete rSet;
    delete showAll;


}
void deleteOfCard(const string& tableName) {
    int productId;
    cout << "Enter Product ID to delete from cart: ";
    cin >> productId;

    try {
        string selectSQL = "SELECT * FROM " + tableName + " WHERE id = ?";
        PreparedStatement* pstmt = con->prepareStatement(selectSQL);
        pstmt->setInt(1, productId);
        ResultSet* rSet = pstmt->executeQuery();

        if (!rSet->next()) {
            cout << "Product not found in cart." << endl;
            delete rSet;
            delete pstmt;
            return;
        }

        string product_name = rSet->getString("product_name");
        int price = rSet->getInt("price");
        int quantity = rSet->getInt("quantity");
        string color = rSet->getString("color");

        delete rSet;
        delete pstmt;

        PreparedStatement* insert = con->prepareStatement(
            "INSERT INTO product(name, price, quantity, color) VALUES (?, ?, ?, ?)"
        );
        insert->setString(1, product_name);
        insert->setInt(2, price);
        insert->setInt(3, quantity);
        insert->setString(4, color);
        insert->executeUpdate();
        delete insert;

        PreparedStatement* deleteFromCard = con->prepareStatement(
            "DELETE FROM " + tableName + " WHERE id = ?"
        );
        deleteFromCard->setInt(1, productId);
        int rows = deleteFromCard->executeUpdate();
        delete deleteFromCard;

        if (rows > 0)
            cout << "Product removed from cart successfully." << endl;

    } catch (sql::SQLException &e) {
        cerr << "error deleteOfCard: " << e.what() << endl;
    }
}
void clearCard(const string& tableName)
{
    string sql = "DELETE FROM " + tableName;
    Statement* stmt = con->createStatement();
    stmt->executeUpdate(sql);
    delete stmt;
    cout << endl << "Payment was OK" << endl;
}
int main() {
    int choice = -1;
    char condition = 'y';
    int subChoice = 0;

    Driver* driver = get_driver_instance();
    con = driver->connect("tcp://127.0.0.1:3306", "root", "MyNewPassword123");
    con->setSchema("shop");

    auto [mainId, mainUsername] = login(con);
    if (mainId == -1 || mainUsername.empty()) return 1;

    string tableName = mainUsername + "s_table";

    do {
        cout << "1. All Products and Add to cart" << endl;
        cout << "2. All items in your cart" << endl;
        cout << "3.Exit" << endl;
        
        while (true) {
            cout << "Choose an option: ";
            cin >> choice;
            

            if (!cin.fail()) {
                break;
            }

            cout << "enter valid option" << endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }

        switch (choice) {
            case 1:
                deleteZeroProductQuantity();
                sumProduct(sumOfProducts);
                if (sumOfProducts == 0) {
                    PreparedStatement* updateId = con->prepareStatement("ALTER TABLE product AUTO_INCREMENT = 1");
                    updateId->executeUpdate();
                    delete updateId;
                    cout << "Oh sorry! products available is done!" << endl << endl;
                    break;
                }
                cout << "All products:" << endl;
                allProduct();

                cout << endl << "1. Add to cart" << endl;
                cout << "2. Go Home" << endl;
                while (true) {
                    cout << "Choose an option: ";
                    cin >> subChoice;

                    if (!cin.fail()) {
                        break;
                    }
                
                    cout << "enter valid option" << endl;
                    cin.clear();
                    cin.ignore(1000, '\n');
                }
                switch (subChoice) {
                    case 1:
                        creatCard(tableName, mainId);
                        addToCard(tableName);
                        break;
                    case 2:
                        condition = 'y';
                        break;
                    default:
                        cout << "Choose a valid option!" << endl;
                        break;
                }
                break;

            case 2:
                allCartItems(tableName);
                cout << endl << "1. Payment the cart" << endl;
                cout << "2. Delete a product from cart" << endl;
                cout << "3. Go to Home" << endl;
                while (true) {
                    
                    cout << "Choose an option: ";
                    cin >> subChoice;
                    if (!cin.fail()) {
                        break;
                    }
                
                    cout << "enter valid option" << endl;
                    cin.clear();
                    cin.ignore(1000, '\n');
                }
                switch (subChoice) {
                    case 1:
                    clearCard(tableName);
                    cout << "Payment done." << endl;
                    if (1 == 1)
                    {
                        string updateTableId = "ALTER TABLE "+ tableName +" AUTO_INCREMENT = 1";
                        PreparedStatement* updateId = con->prepareStatement(updateTableId);
                        updateId->executeUpdate();
                        delete updateId;
                    }
                        
                        break;
                    case 2:
                        deleteOfCard(tableName);
                        break;
                    case 3:
                        condition = 'y';
                        break;
                    default:
                        cout << "Choose a valid option!" << endl;
                        break;
                }
                break;
            case 3:
            condition = 'n';
            break;
            default:
                cout << "Choose a valid option!" << endl;
                break;
        }

    } while (condition == 'y');

    delete con;
    return 0;
}
