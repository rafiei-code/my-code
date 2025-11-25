#include "httplib.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include "BCrypt.hpp"
#include <random>

using namespace std;
using namespace sql;
using namespace httplib;
using json = nlohmann::json;

Connection* con ;

string generateToken(int length = 32) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    random_device rd;

    string token;
    for(int i=0; i<length; ++i)
        token += chars[rd() % sizeof(chars) - 2];
    return token;
}

tuple<string,string,string> findUser(Connection* con, const string& username) {
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT username, password, role FROM users WHERE username = ?");
        pstmt->setString(1, username);
        ResultSet* res = pstmt->executeQuery();
        if(res->next()) {
            string u = res->getString("username");
            string hash = res->getString("password");
            string role = res->getString("role");
            delete res;
            delete pstmt;
            return make_tuple(u, hash, role);
        }
        delete res;
        delete pstmt;
        return make_tuple("", "", "");
    } catch(SQLException &e) {
        cout << "SQL Error: " << e.what() << endl;
        return make_tuple("", "", "");
    }
}

void signUp(const Request& req, Response& res) {
    try {
        json j = json::parse(req.body);
        string username = j.value("username", "");
        string password = j.value("password", "");

        if(username.empty() || password.empty()) {
            res.status = 400;
            res.set_content("{error : username or password are not valid}", "application/json");
            return;
        }
        string hashed = BCrypt::generateHash(password);

        PreparedStatement* pstmt = con->prepareStatement("INSERT INTO users(username, password, role) VALUES(?, ?, ?)");
        pstmt->setString(1, username);
        pstmt->setString(2, hashed);
        pstmt->setString(3, "user");
        pstmt->executeUpdate();
        delete pstmt;

        res.status = 201;
        res.set_content("{message : user registered successfully}", "application/json");

    } catch(SQLException &e) {
        cout << "SQL Error: " << e.what() << endl;
        res.status = 500;
        res.set_content("{error:DB error}", "application/json");
    }
}

void login(const Request& req, Response& res) {
    try {
        json j = json::parse(req.body);
        string username = j.value("username", "");
        string password = j.value("password", "");

        if(username.empty() || password.empty()) {
            res.status = 400;
            res.set_content("{error : username or password are not valid}", "application/json");
            return;
        }

        auto [u, hash, role] = findUser(con, username);
        if(u.empty()) {
            res.status = 401;
            res.set_content("{error : user not found}", "application/json");
            return;
        }

        if(BCrypt::validatePassword(password, hash)) {
            string token = generateToken();
            json response = { {"success", true}, {"role", role}, {"message", "Login successful"}, {"token", token}};
            res.status = 200;
            res.set_content(response.dump(), "application/json");
        } else {
            res.status = 401;
            res.set_content("{error : password is not valid}", "application/json");
        }

    } catch(SQLException &e) {
        cout << "SQL Error: " << e.what() << endl;
        res.status = 500;
        res.set_content("{ error :DB error}", "application/json");
    }
}

int main() {
    try {
        Driver* driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "testuser", "123456");
        con->setSchema("testdb");

        Server server;
        server.Post("/signup", signUp);
        server.Post("/login", login);

        cout << "Server starting ..." << endl;
        server.listen("127.0.0.1", 8080);

    } catch(SQLException &e) {
        cout << "Database connection error: " << e.what() << endl;
    }
}
