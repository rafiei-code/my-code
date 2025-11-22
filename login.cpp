#include <iostream>
#include <memory>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include "bcrypt.h"
#include "BCrypt.hpp" 
#include <tuple>
using namespace std;
using namespace sql;
tuple<string,string,string>findUser(Connection* con,const string& username)
{
    try{
        PreparedStatement *pstmt( con->prepareStatement("SELECT username,password,role FROM users WHERE username = ? "));
        pstmt->setString(1,username);
        ResultSet *result(pstmt->executeQuery());
        if (result->next())
        {
            string userName = result->getString("username");
            string hash = result->getString("password");
            string role = result->getString("role");
            return make_tuple(userName,hash,role);
        }
        return make_tuple("","","");
    }catch(SQLException &e)
    {
        cout << "Error : " <<  e.what() << endl;
        return make_tuple("","","");

    }
    
}
void login(Connection *con,string &usern,const string &password)
{
    auto [u, hash, role] = findUser(con,usern);
    if (u.empty())
    {
        cout << "user not found!" << endl;
        return;
    }
    if (BCrypt::validatePassword(password,hash))
    {
        cout << "Login was successful" << endl << "role : " << role << endl;
    }else{
        cout << "password was not valid!" << endl;
    }
    
}
int main()
{
    Driver *driver = get_driver_instance();
    Connection *con = driver->connect("tcp://127.0.0.1","testuser","123456");
    con->setSchema("testdb");
    string username,password;
    cout << "Enter userName : ";
    cin >> username;
    cout << "Enter password : ";
    cin >> password;
    login(con,username,password);
}