#include "httplib.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include "BCrypt.hpp"
#include <random>
#include <jwt-cpp/jwt.h>

using namespace std;
using namespace sql;
using namespace httplib;
using json = nlohmann::json;
//using namespace jwt;
Connection* con ;
const std::string JWT_SECRET = "MY_SUPER_SECRET_KEY";

// string generateToken(int length = 32) {
//     static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

//     random_device rd;

//     string token;
//     for(int i=0; i<length; ++i)
//         token += chars[rd() % sizeof(chars) - 2];
//     return token;
// }

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
        auto [user,pass,role] = findUser(con,username);
        if (username != user)
        {
            PreparedStatement* pstmt = con->prepareStatement("INSERT INTO users(username, password, role) VALUES(?, ?, ?)");
            pstmt->setString(1, username);
            pstmt->setString(2, hashed);
            pstmt->setString(3, "user");
            pstmt->executeUpdate();
            delete pstmt;
    
            res.status = 201;
            res.set_content("{message : user registered successfully}", "application/json");
        }
        else{
            res.status = 401;
            res.set_content("{message : user registered was not successfully}", "application/json");
        }

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

            auto token = jwt::create()
                .set_issuer("auth_service")
                .set_type("jwt")
                .set_subject("login_token")
                .set_payload_claim("username", jwt::claim(username))
                .set_payload_claim("password",jwt::claim(password))
                .set_payload_claim("role",jwt::claim(role))
                .set_expires_at(chrono::system_clock::now()+chrono::hours(24))
                .sign(jwt::algorithm::hs256{JWT_SECRET});

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
void allUsers(const Request& req, Response& res) {
    string username, role,password;
    auto auth = req.get_header_value("Authorization");
    if(auth.empty() || auth.find("Bearer ") != 0) {
        res.status = 401;
        res.set_content("{ error : Unauthorized }", "application/json");
        return;
    }

    string token = auth.substr(7);
    try {
        auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET})
            .with_issuer("auth_service")
            .verify(decoded);

        username = decoded.get_payload_claim("username").as_string();
        password = decoded.get_payload_claim("password").as_string();
        role = decoded.get_payload_claim("role").as_string();

    } catch(...) {
        res.status = 401;
        res.set_content("{ error : Invalid token }", "application/json");
        return;
    }

    json users = json::array();
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT username, role FROM users");
        ResultSet* rSet = pstmt->executeQuery();
        while(rSet->next()) {
            users.push_back({
                {"username", rSet->getString("username")},
                {"role", rSet->getString("role")}
            });
        }
        delete rSet;
        delete pstmt;

        res.status = 200;
        res.set_content(users.dump(), "application/json");
    } catch(sql::SQLException &e) {
        res.status = 500;
        res.set_content("{ error : Database query failed }", "application/json");
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
        server.Get("/users", allUsers);

        cout << "Server starting ..." << endl;
        server.listen("0.0.0.0", 8080);

    } catch(SQLException &e) {
        cout << "Database connection error: " << e.what() << endl;
    }
}