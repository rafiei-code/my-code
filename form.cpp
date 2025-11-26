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

Connection* con ;

const string jwtSecretAccess = "MY_ACCESS_SECRET";
const string jwtSecretRefresh = "MY_REFRESH_SECRET";


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
string generateAccessToken(const string& username,const string& role)
{
    return jwt::create()
        .set_issuer("auth_service")
        .set_type("jwt")
        .set_subject("access_token")
        .set_payload_claim("username", jwt::claim(username))
        .set_payload_claim("role",jwt::claim(role))
        .set_expires_at(chrono::system_clock::now()+chrono::hours(1))
        .sign(jwt::algorithm::hs256{jwtSecretAccess});

}
string generateRefreshToken(const string& username) {
    return jwt::create()
        .set_issuer("auth_service")
        .set_type("jwt")
        .set_subject("refresh_token")
        .set_payload_claim("username", jwt::claim(username))
        .set_expires_at(chrono::system_clock::now() + chrono::hours(24*10))
        .sign(jwt::algorithm::hs256{jwtSecretRefresh});
}
bool verifyAccessToken(string& username,string& role,const string& token)
{
    try {
        auto decoded = jwt::decode(token);
        jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{jwtSecretAccess})
        .with_issuer("auth_service")
        .verify(decoded);
        
        username = decoded.get_payload_claim("username").as_string();
        role = decoded.get_payload_claim("role").as_string();
        return true;
    } catch(...) {
        return false;
    }
    
}
bool verifyRefreshToken(string& username,const string& token)
{
    try {
        auto decoded = jwt::decode(token);
        jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{jwtSecretRefresh})
        .with_issuer("auth_service")
        .verify(decoded);
        
        username = decoded.get_payload_claim("username").as_string();  
        return true; 
    } catch(...) {
        return false;
    }
    
}
void refreshToken(const Request& req, Response& res) {
    try {
        json j = json::parse(req.body);
        string refreshToken = j.value("refresh_token", "");

        if(refreshToken.empty()) {
            res.status = 400;
            res.set_content("{ error : refresh token required }", "application/json");
            return;
        }

        string username;
        string role;

        if(!verifyRefreshToken(username, refreshToken)) {
            res.status = 401;
            res.set_content("{ error : invalid or expired refresh token }","application/json");
            return;
        }

        try{
            PreparedStatement *pstmt = con->prepareStatement("SELECT refresh_token,expires_at FROM refresh_tokens WHERE username = ? AND refresh_token = ?");
            pstmt->setString(1,username);
            pstmt->setString(2,refreshToken);
            ResultSet* rSet = pstmt->executeQuery();
            if (!rSet->next())
            {
                delete pstmt;
                delete rSet;
                res.status = 401;
                res.set_content("{ error : refresh token not found or revoked }", "application/json");
                return;
            }
            delete pstmt;
            delete rSet;
            
        }catch(SQLException &e)
        {
            cout << "SQL ERROR : " << e.what() << endl;
            res.status = 500;
            res.set_content("{ error : DB error}", "application/json");
            return;
        }
        {
            auto [user,pass,rolee] = findUser(con,username);
            role = rolee.empty() ? "user" : rolee;
        }
        
        string newAccessToken = generateAccessToken(username, role);
        json response = { {"access_token", newAccessToken}, {"message", "new access token generated"}, {"success", true}};

        res.status = 200;
        res.set_content(response.dump(), "application/json");

    } catch(...) {
        res.status = 500;
        res.set_content("{error : server error}", "application/json");
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
            PreparedStatement* del = con->prepareStatement("DELETE FROM refresh_tokens WHERE username = ?");
            del->setString(1, username);
            del->executeUpdate();
            delete del;
            string accessToken = generateAccessToken(username,role);
            string refreshToken = generateRefreshToken(username);
            PreparedStatement* pstmt = con->prepareStatement("INSERT INTO refresh_tokens(username, refresh_token, expires_at) VALUES (?, ?, ?)");
            pstmt->setString(1, username);
            pstmt->setString(2, refreshToken);
            pstmt->setString(3, "2030-01-01 00:00:00");
            pstmt->executeUpdate();
            delete pstmt;
            json response = { {"success", true}, {"role", role}, {"message", "Login successful"}, {"access_token", accessToken},{"refresh_token",refreshToken}};
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
    if (!verifyAccessToken(username,role,token))
    {
        res.status = 401;
        res.set_content("{error : Invalid or expired token}","application/json");
        return;
    }
    
    json users = json::array();
    if (role == "admin")
    {
        try {
            PreparedStatement* pstmt = con->prepareStatement("SELECT username, role FROM users");
            ResultSet* rSet = pstmt->executeQuery();
            while(rSet->next()) {
                users.push_back({ {"username", rSet->getString("username")}, {"role", rSet->getString("role")}});
            }
            delete rSet;
            delete pstmt;
    
            res.status = 200;
            res.set_content(users.dump(), "application/json");
        } catch(sql::SQLException &e) {
            res.status = 500;
            res.set_content("{ error : Database query failed }", "application/json");
        }
    }else{
        res.status = 403;
        res.set_content("{ error : you dont have access }", "application/json");
    }
    
}
int main() {
    //hiiiii
    try {
        Driver* driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "testuser", "123456");
        con->setSchema("testdb");

        Server server;
        server.Post("/signup", signUp);
        server.Post("/login", login);
        server.Post("/refresh", refreshToken);
        server.Get("/users", allUsers);

        cout << "Server starting ..." << endl;
        server.listen("0.0.0.0", 8080);

    } catch(SQLException &e) {
        cout << "Database connection error: " << e.what() << endl;
    }
}