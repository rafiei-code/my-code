#include <iostream>
#include <cstdlib>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
using namespace std;
using namespace sql;

int main() {
    const char* env = getenv("ENV");
    string dbHost, dbName, dbUser, dbPass;

    if (env && string(env) == "production") {
        dbHost = "tcp://127.0.0.1:3306";
        dbName = "testdb";
        dbUser = "testuser";
        dbPass = "123456";
        cout << "Running in Production environment" << endl;
    } else {
        dbHost = "127.0.0.1";
        dbName = "todoList";
        dbUser = "root";
        dbPass = "123456";
        cout << "Running in Sandbox environment" << endl;
    }

    try {
        Driver* driver = get_driver_instance();
        Connection* con = driver->connect(dbHost, dbUser, dbPass);
        con->setSchema(dbName);
        PreparedStatement* pstmt = con->prepareStatement("SELECT username, role FROM users");

        cout << "Connected to database: " << dbName << endl;
        delete con;
    } catch (SQLException &e) {
        cerr << "Error connecting to database: " << e.what() << endl;
    }

    return 0;
}
