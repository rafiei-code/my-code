#include "mainHeader.h"
#include "findUser.h"
void signUp(const Request& req, Response& res) {
    extern Connection *con;
    try {
        json j = json::parse(req.body);
        string username = j.value("username", "");
        string password = j.value("password", "");
        string passwordConfirm = j.value("password_confirm", "");

        if(username.empty() || password.empty() ) {
            res.status = 400;
            res.set_content("{error : username or password are not valid}", "application/json");
            return;
        }
        if (passwordConfirm != password)
        {
            res.status = 400;
            res.set_content("{error :Password does not match the password confirmation }", "application/json");
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