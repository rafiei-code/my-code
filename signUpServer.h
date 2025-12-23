#include "findUser.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

inline json signUpHttp(sql::Connection* con, const std::string& username, const std::string& password, const std::string& confirm, const std::string& email) {
    json res;

    // بررسی فیلدها
    if (username.empty() || password.empty() || confirm.empty() || email.empty()) {
        res["status"] = "error";
        res["message"] = "All fields are required";
        return res;
    }

    if (password != confirm) {
        res["status"] = "error";
        res["message"] = "Passwords do not match";
        return res;
    }

    if (email.length() < 10 || email.substr(email.length() - 10) != "@gmail.com") {
        res["status"] = "error";
        res["message"] = "Invalid email";
        return res;
    }

    // بررسی وجود کاربر
    auto [u, pass] = findUser(con, username);
    if (!u.empty()) {
        res["status"] = "error";
        res["message"] = "Username already exists";
        return res;
    }

    try {
        // درج کاربر
        PreparedStatement* pstmt = con->prepareStatement("INSERT INTO users(name,email,password) VALUES(?,?,?)");
        pstmt->setString(1, username);
        pstmt->setString(2, email);
        pstmt->setString(3, password);
        pstmt->executeUpdate();

        // گرفتن id
        PreparedStatement* selectId = con->prepareStatement("SELECT id FROM users WHERE name = ?");
        selectId->setString(1, username);
        ResultSet* rSet = selectId->executeQuery();
        int id = -1;
        if (rSet->next()) id = rSet->getInt("id");

        PreparedStatement* addValet = con->prepareStatement("INSERT INTO valet(user_id) VALUES(?)");
        addValet->setInt(1, id);
        addValet->executeUpdate();

        delete pstmt;
        delete selectId;
        delete addValet;
        delete rSet;

        res["status"] = "ok";
        res["id"] = id;
        res["username"] = username;
    } catch (sql::SQLException& e) {
        res["status"] = "error";
        res["message"] = e.what();
    }

    return res;
}
