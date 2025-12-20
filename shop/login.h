#include "findUser.h"
#include <tuple>
#include <termios.h>
#include <unistd.h>
inline tuple <int,string> login(Connection* con) {
    int id;
    string mainUsername;
    string username,password;
    cout << "enter username : ";
    cin >> username;

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    cout << "Password: ";
    cin >> password;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    cout << endl;

    cout << "Done" << endl;
    auto [u, pass] = findUser(con, username);
    if(u.empty() || pass.empty() || u != username || pass != password) {
        cout << "Enter valid  or paassword !" << endl;
        return make_tuple(-1, "");
    }
    PreparedStatement *pstmt = con->prepareStatement("SELECT id, name FROM users WHERE name = ?");
    pstmt->setString(1, username);
    ResultSet *rSet = pstmt->executeQuery();
    if(rSet->next()) {
        id = rSet->getInt("id");
        mainUsername = rSet->getString("name");
    } else {
        cout << "User not found in database!" << endl;
        delete rSet;
        delete pstmt;
        return make_tuple(-1, "");
    }

    delete pstmt;
    delete rSet;
    return make_tuple (id,mainUsername);
    
}