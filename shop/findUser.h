#pragma once 
#include <tuple>
#include <string>
#include <cppconn/connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

using namespace std;
using namespace sql;
inline tuple<string,string> findUser(Connection* con, const string& username) {
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT name, password FROM users WHERE name = ?");
        pstmt->setString(1, username);
        ResultSet* res = pstmt->executeQuery();
        if(res->next()) {
            string u = res->getString("name");
            string pass = res->getString("password");
            delete res;
            delete pstmt;
            return make_tuple(u, pass);
        }
        delete res;
        delete pstmt;
        return make_tuple("","");
    } catch(SQLException &e) {
        cout << "SQL Error: " << e.what() << endl;
        return make_tuple("","");
    }
}