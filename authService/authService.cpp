#include "mainHeader.h"
#include "signUp.h"
#include "login.h"
#include "refreshToken.h"
#include "deleteUsersAccess.h"
#include "changeUsersRoleAccess.h"
#include "allUsersAccess.h"
#include <cstdlib>
sql::Connection* con = nullptr;
const string jwtSecretAccess = "MY_ACCESS_SECRET";
const string jwtSecretRefresh = "MY_REFRESH_SECRET";




int main() {
    try {
//hi

        const char* env = getenv("ENV");
        string dbHost, dbName, dbUser, dbPass;

        if (env && string(env) == "production") {
            dbHost = "tcp://127.0.0.1:3306";
            dbName = "testdb";
            dbUser = "testuser";
            dbPass = "123456";
            cout << "Running in Production environment" << endl;
        } else if(env && string(env) == "sandbox") {
            dbHost = "127.0.0.1";
            dbName = "todoList";
            dbUser = "root";
            dbPass = "123456";
            cout << "Running in Sandbox environment" << endl;
        }else{
            cout << "enter the API Key " << endl;
            return 1;
        }

        Driver* driver = get_driver_instance();
        Connection* con = driver->connect(dbHost, dbUser, dbPass);
        con->setSchema(dbName);

        Server server;
        server.Post("/signup", signUp);
        server.Post("/login", login);
        server.Post("/refresh", refreshToken);
        server.Delete(R"(/users/.*)", deleteUser);
        server.Put(R"(/users/.*)", changeUserRole);
        server.Get("/users", allUsers);

        cout << "Server starting ..." << endl;
        server.listen("0.0.0.0", 8080);

    } catch(SQLException &e) {
        cout << "Database connection error: " << e.what() << endl;
    }
}