#include <iostream>
#include <memory>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include "bcrypt.h"
#include "BCrypt.hpp"

using namespace std;
using namespace sql;

void signUp(Connection* con, const string& username, const string& password) {
    string hash = BCrypt::generateHash(password);

    try {
        PreparedStatement *pstmt(con->prepareStatement("INSERT INTO users(username, password, role) VALUES(?, ?, ?)"));
        pstmt->setString(1, username);
        pstmt->setString(2, hash);
        pstmt->setString(3, "user");
        pstmt->executeUpdate();

        cout << "User add was successfull" << endl;

    } catch (SQLException &e) {
        cout << "Error: " << e.what() << endl;
    }
}
int main() {
    try {
        Driver* driver = get_driver_instance();
        Connection *con = driver->connect("tcp://127.0.0.1:3306", "testuser", "123456");

        con->setSchema("testdb");

        string username, password;
        cout << "Enter username: "; cin >> username;
        cout << "Enter password: "; cin >> password;

        signUp(con, username, password);

    } catch (SQLException &e) {
        cout << "Database error: " << e.what() << endl;
    }
}
