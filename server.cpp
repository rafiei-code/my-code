#include "httplib.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "BCrypt.hpp"

using namespace sql;
using namespace std;
using namespace httplib;

using json = nlohmann::json;

Connection* con = nullptr;

void handleGet(const Request& req, Response& res) {
    json j;
    j["users"] = json::array();

    try {
        Statement* stmt = con->createStatement();
        ResultSet* result = stmt->executeQuery("SELECT * FROM users");

        while (result->next()) {
            json user;
            user["id"] = result->getInt("id");
            user["username"] = result->getString("username");
            user["password"] = result->getString("password");
            user["role"] = result->getString("role");
            j["users"].push_back(user);
        }

        delete result;
        delete stmt;

        res.set_content(j.dump(4), "application/json");

    } catch (SQLException& e) {
        cout << "SQL ERROR: " << e.what() << endl;
        res.status = 500;
        res.set_content("{\"error\":\"DB error\"}", "application/json");
    }
}

void handlePost(const Request& req, Response& res) {
    cout << "POST body: " << req.body << endl;

    try {
        json j = json::parse(req.body);

        string username = j.value("username", "");
        string password = j.value("password", "");
        string role = j.value("role", "user");

        if (username.empty() || password.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"username or password missing\"}", "application/json");
            return;
        }

        string hashed = BCrypt::generateHash(password);
        cout << "Inserting user: " << username << endl;

        PreparedStatement* pstmt = con->prepareStatement(
            "INSERT INTO users(username,password,role) VALUES(?,?,?)"
        );
        pstmt->setString(1, username);
        pstmt->setString(2, hashed);
        pstmt->setString(3, role);
        pstmt->executeUpdate();
        delete pstmt;

        res.status = 201;
        res.set_content("{\"message\":\"user created\"}", "application/json");

    } catch (SQLException& e) {
        cout << "SQL ERROR: " << e.what() << endl;
        res.status = 500;
        res.set_content("{\"error\":\"DB error\"}", "application/json");
    } catch (nlohmann::json::exception& e) {
        cout << "JSON ERROR: " << e.what() << endl;
        res.status = 400;
        res.set_content("{\"error\":\"Invalid JSON\"}", "application/json");
    } catch (...) {
        res.status = 500;
        res.set_content("{\"error\":\"Unknown error\"}", "application/json");
    }
}

int main() {
    try {
        Driver* driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "testuser", "123456");
        con->setSchema("testdb");

        Server server;
        server.Get("/users", handleGet);
        server.Post("/users", handlePost);

        cout << "Server starting on 0.0.0.0:8080 ..." << endl;
        server.listen("127.0.0.1", 8080);

    } catch (SQLException& e) {
        cout << "DB connection error: " << e.what() << endl;
        return 1;
    }
}
