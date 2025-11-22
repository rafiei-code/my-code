#include <iostream>
#include <memory>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "BCrypt.hpp"
#include "httplib.h"
#include <nlohmann/json.hpp>
using namespace std;
using namespace sql;
using json = nlohmann::json;

tuple<string,string,string> findUser(Connection* con, const string& username) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT username,password,role FROM users WHERE username = ?")
        );
        pstmt->setString(1, username);
        unique_ptr<ResultSet> result(pstmt->executeQuery());

        if (result->next()) {
            return make_tuple(
                result->getString("username"),
                result->getString("password"),
                result->getString("role")
            );
        }
        return make_tuple("", "", "");

    } catch (SQLException &e) {
        cout << "DB Error : " << e.what() << endl;
        return make_tuple("", "", "");
    }
}


void handleGet(Connection* con, const httplib::Request &req, httplib::Response &res) {
    json j;
    j["users"] = json::array();

    try {
        unique_ptr<Statement> stmt(con->createStatement());
        unique_ptr<ResultSet> result(stmt->executeQuery("SELECT id, username, role FROM users"));

        while (result->next()) {
            json u;
            u["id"] = result->getInt("id");
            u["username"] = result->getString("username");
            u["role"] = result->getString("role");
            j["users"].push_back(u);
        }

        res.status = 200;
        res.set_content(j.dump(4), "application/json");

    } catch (SQLException &e) {
        res.status = 500;
        res.set_content("{\"message\":\"database error\"}", "application/json");
    }
}

void handleGetOne(Connection* con, const httplib::Request &req, httplib::Response &res, int id) {
    json j;

    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("SELECT id, username, role FROM users WHERE id=?")
        );
        pstmt->setInt(1, id);
        unique_ptr<ResultSet> result(pstmt->executeQuery());

        if (!result->next()) {
            res.status = 404;
            res.set_content("{\"message\":\"user not found\"}", "application/json");
            return;
        }

        j["id"] = result->getInt("id");
        j["username"] = result->getString("username");
        j["role"] = result->getString("role");

        res.status = 200;
        res.set_content(j.dump(4), "application/json");

    } catch (SQLException &e) {
        res.status = 500;
        res.set_content("{\"message\":\"database error\"}", "application/json");
    }
}

void handlePost(Connection* con, const httplib::Request &req, httplib::Response &res) {
    auto j = json::parse(req.body);

    string username = j.value("username", "");
    string password = j.value("password", "");
    string role = j.value("role", "");

    if (username.empty() || password.empty()) {
        res.status = 400;
        res.set_content("{\"message\":\"username and password required\"}", "application/json");
        return;
    }

    string hashed = BCrypt::generateHash(password);

    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("INSERT INTO users(username,password,role) VALUES(?,?,?)")
        );
        pstmt->setString(1, username);
        pstmt->setString(2, hashed);
        pstmt->setString(3, role);
        pstmt->execute();

        res.status = 201;
        res.set_content("{\"message\":\"user created\"}", "application/json");

    } catch (SQLException &e) {
        res.status = 500;
        res.set_content("{\"message\":\"database error\"}", "application/json");
    }
}

void handleDelete(Connection* con, int id, httplib::Response &res) {
    try {
        unique_ptr<PreparedStatement> pstmt(
            con->prepareStatement("DELETE FROM users WHERE id=?")
        );
        pstmt->setInt(1, id);

        if (pstmt->executeUpdate() == 0) {
            res.status = 404;
            res.set_content("{\"message\":\"user not found\"}", "application/json");
            return;
        }

        res.status = 200;
        res.set_content("{\"message\":\"user deleted\"}", "application/json");

    } catch (SQLException &e) {
        res.status = 500;
        res.set_content("{\"message\":\"database error\"}", "application/json");
    }
}


int main() {
    Driver *driver = get_driver_instance();
    Connection *con = driver->connect("tcp://127.0.0.1", "testuser", "123456");
    con->setSchema("testdb");

    httplib::Server server;

    // GET ALL
    server.Get("/user", [&](const httplib::Request &req, httplib::Response &res) {
        handleGet(con, req, res);
    });

    // GET ONE
    server.Get(R"(/user/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
        int id = stoi(req.matches[1]);
        handleGetOne(con, req, res, id);
    });

    // POST
    server.Post("/user", [&](const httplib::Request &req, httplib::Response &res) {
        handlePost(con, req, res);
    });

    // DELETE
    server.Delete(R"(/user/(\d+))", [&](const httplib::Request &req, httplib::Response &res) {
        int id = stoi(req.matches[1]);
        handleDelete(con, id, res);
    });

    cout << "Server started on port 8080" << endl;
    server.listen("0.0.0.0", 8080);
}
