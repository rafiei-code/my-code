#include "findUser.h"
#include <nlohmann/json.hpp>
using json = nlohmann::json;

inline json loginHttp(sql::Connection* con, const std::string& username, const std::string& password) {
    json res;

    auto [u, pass] = findUser(con, username);
    if (u.empty() || pass.empty() || u != username || pass != password) {
        res["status"] = "error";
        res["message"] = "Invalid username or password";
        return res;
    }

    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT id, name FROM users WHERE name = ?");
        pstmt->setString(1, username);
        ResultSet* rSet = pstmt->executeQuery();
        int id = -1;
        std::string mainUsername;

        if (rSet->next()) {
            id = rSet->getInt("id");
            mainUsername = rSet->getString("name");
        } else {
            res["status"] = "error";
            res["message"] = "User not found";
            delete pstmt;
            delete rSet;
            return res;
        }

        delete pstmt;
        delete rSet;

        res["status"] = "ok";
        res["id"] = id;
        res["username"] = mainUsername;
    } catch (sql::SQLException& e) {
        res["status"] = "error";
        res["message"] = e.what();
    }

    return res;
}
