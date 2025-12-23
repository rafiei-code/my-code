#include "findUser.h"
#include <tuple>
#include <termios.h>
#include <unistd.h>
void signUp(Connection* con) {
    int id;
    bool flag = true;
    string username,password,confirm,email;
    cout << "enter username : ";
    cin >> username;

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    cout << "Password : ";
    cin >> password;
    cout << endl << "enter password cinfirm : ";
    cin >> confirm;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    cout << endl;
    while (true) {
        cout << "enter new email : ";
        cin >> email;
        
        if (!cin.fail()) {
            break;
        }
        cout << "enter email !" << endl;
        cin.clear();
        cin.ignore(1000, '\n');
    }
    if (email.length() >= 10) {
        string gmailEndPoint = email.substr(email.length() - 10);
        if (gmailEndPoint != "@gmail.com") {
            cout << "email not valid !" << endl;
        }
    }else if(email.length() < 10){
        cout << "email not valid !" << endl;
    }else{
        flag = false;
    }
    cout << "Done" << endl;
    auto [u, pass] = findUser(con, username);
    if(password.empty() || u == username || username.empty()) {
        cout << "Enter valid  username or paassword !" << endl;
    }
    else if (!u.empty())
    {
        cout << "this username used" << endl;
        cout << "please chose other username" << endl;
    }
    else{

        PreparedStatement *pstmt = con->prepareStatement("INSERT INTO users(name,email,password) VALUES(?,?,?)");
        pstmt->setString(1, username);
        pstmt->setString(2, email);
        pstmt->setString(3, password);
        pstmt->executeUpdate();
        
        PreparedStatement *selectId = con->prepareStatement("SELECT id FROM users WHERE name = ?");
        selectId->setString(1, username);
        ResultSet *rSet = selectId->executeQuery();
        if (rSet->next())
        {
            id = rSet->getInt("id");
        }
        PreparedStatement *addValet = con->prepareStatement("INSERT INTO valet(user_id) VALUES(?)");
        addValet->setInt(1, id);
        addValet->executeUpdate();
    
        delete pstmt; 
        delete addValet;
        delete rSet;   
    }
}