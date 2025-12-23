#include <iostream>
#include <jwt-cpp/jwt.h>
#include <nlohmann/json.hpp>
#include "httplib.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <ctime>
#include "BCrypt.hpp"
#include <iomanip>
#include "login.h"
#include <random>
#include <string>
#include "signUp.h"
using namespace sql;
using namespace std;
using namespace httplib;
using json = nlohmann::json;

Connection *con;
void getValet(int);
void sumProduct(int &value)
{
    value = 0;
    PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM product");
    ResultSet* rSet = pstmt->executeQuery();
    while(rSet->next()) {
        int mainQuantity = rSet->getInt("quantity");            
        value += 1;
    }
    delete rSet;
    delete pstmt;

}
void deleteZeroProductQuantity() {
    try {
        PreparedStatement* deleteProduct = con->prepareStatement("DELETE FROM product WHERE quantity = 0");
        deleteProduct->executeUpdate();
        delete deleteProduct;
    } catch(sql::SQLException &e) {
        cerr << "SQL Error (deleteZeroProductQuantity): " << e.what() << endl;
    }
}
void deleteZeroProductQuantityFromCard(const string& tableName) {
    try {
        string sql = "DELETE FROM " + tableName + " WHERE quantity = 0";
        PreparedStatement* deleteProduct = con->prepareStatement(sql);
        deleteProduct->executeUpdate();
        delete deleteProduct;
    } catch(sql::SQLException &e) {
        cerr << "SQL Error (deleteZeroProductQuantity): " << e.what() << endl;
    }
}
void allProduct()
{ 
    try {
        PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM product");
        ResultSet* rSet = pstmt->executeQuery();
        cout << left << setw(5)  << "ID" << setw(20) << "Name" << setw(14) << "Price" << setw(12) << "Quantity" << setw(10) << "Color" << endl;
        
        cout << string(60, '-') << endl;
        
        while(rSet->next()) {
            cout << left << setw(5)  << rSet->getInt("id") << setw(20) << rSet->getString("name") << setw(15) << fixed << rSet->getInt("price") << setw(12) << rSet->getInt("quantity") << setw(10) << rSet->getString("color") << endl;
        }

        delete rSet;
        delete pstmt;
        
    } catch(sql::SQLException &e) {
        cerr << "SQL Error (allProduct): " << e.what() << endl;
    }
}
void creatCart(const string& tableName,int id)
{
    
    try {
        bool hadCart = true;
        PreparedStatement *finalT = con->prepareStatement("SELECT had_cart FROM users WHERE id = ?");
        finalT->setInt(1,id);
        ResultSet *rSet = finalT->executeQuery();
        if(rSet->next()) {
            hadCart = rSet->getBoolean("had_cart");
        } else {
            cout << "User not found!" << endl;
        }

        delete rSet;
        delete finalT;
        if (hadCart == 1)
        {
            return;
        }
        
        string query = "CREATE TABLE " + tableName + " ("
        "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, "
        "product_id INT NOT NULL, "
        "quantity INT NOT NULL, "
        "color VARCHAR(50), "
        "price INT, "
        "final_price DOUBLE, "
        "product_name VARCHAR(50)"
        ")";
        
        Statement* stmt = con->createStatement();
        stmt->execute(query);
        delete stmt;
        
        PreparedStatement *hadCartt = con->prepareStatement("UPDATE users SET had_cart = true WHERE id = ?");
        hadCartt->setInt(1,id);
        hadCartt->executeUpdate();
        cout << "Table " << tableName << " created successfully." << endl;
        
    } catch (SQLException &e) {
        cerr << "error createCart: " << e.what() << endl;
    }
}
void addToCart(const string& tableName)
{
    int userId = 1;
    int productId,quantity;
    bool flag = true;
    while (true) {
        cout << endl << "Enter Product ID : ";
        cin >> productId;

        if (!cin.fail()) {
            break;
        }

        cout << "enter valid product id" << endl;
        cin.clear();
        cin.ignore(1000, '\n');
    }
    
    
    try {
        PreparedStatement* pstmt = con->prepareStatement( "SELECT quantity,color, price, name FROM product WHERE id = ?");
        pstmt->setInt(1, productId);
        ResultSet* rSet = pstmt->executeQuery();
        if (!rSet->next()) {
            cout << "Product not found with this id!" << endl;
            delete rSet;
            delete pstmt;
            return;
        }
        while (true) {
            cout << "Enter quantity : ";
            cin >> quantity;
            
            if (!cin.fail()) {
                break;
            }
        
            cout << "enter valid quantity" << endl;
            cin.clear();
            cin.ignore(1000, '\n');
            }
        int available = rSet->getInt("quantity");
        
        if (available < quantity) {
            cout << "Not enough available : " << available << endl;
            delete rSet;
            delete pstmt;
            return;
        }
        
        string productName = rSet->getString("name");
        string productColor = rSet->getString("color");
        double price = rSet->getDouble("price");
        
        delete rSet;
        delete pstmt;
        
        string sql = "INSERT INTO " + tableName + " (product_id, quantity, color, price, final_price, product_name) VALUES (?,?,?,?,?,?)";
        PreparedStatement* addItem = con->prepareStatement(sql);

        addItem->setInt(1, productId);
        addItem->setInt(2, quantity);   
        addItem->setString(3, productColor);
        addItem->setDouble(4, price);
        addItem->setDouble(5, price * quantity);
        addItem->setString(6, productName);
        addItem->executeUpdate();
        
        delete addItem;
        
        PreparedStatement* updateStmt = con->prepareStatement("UPDATE product SET quantity = quantity - ? WHERE id = ?");
        updateStmt->setInt(1, quantity);
        updateStmt->setInt(2, productId);
        updateStmt->executeUpdate();
        delete updateStmt;
        
        cout << endl << "product added to cart successfully" << endl;
        
        
    } catch (sql::SQLException &e) {
        cerr << "error addToCart : " << e.what() << endl;
    }
}
void createDiscount(string& discount){
    for (int i = 0; i < 5; i++)
    {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> choose(0,1);
        if (choose(gen) == 0)
        {
            uniform_int_distribution<> num(0,9);
            discount += to_string(num(gen));
        }else{
            uniform_int_distribution ch('A','Z');
            discount += static_cast<char>(ch(gen));
        }
    }
    
    cout << "discount code : " << discount << endl;
    PreparedStatement *insertDiscount = con->prepareStatement("INSERT INTO discount (code) VALUES(?)");
    insertDiscount->setString(1,discount);
    insertDiscount->executeUpdate();
    
}
bool verifyDiscountCode(const string& code, const string& accountType){
    try {
        PreparedStatement* pstmt = con->prepareStatement(
            "SELECT id, status, usage_pro, usage_normal FROM discount WHERE code = ?"
        );
        pstmt->setString(1, code);
        ResultSet* res = pstmt->executeQuery();

        if (!res->next()) {
            delete res;
            delete pstmt;
            return false;
        }

        int discountId = res->getInt("id");
        string status = res->getString("status");
        int usagePro = res->getInt("usage_pro");
        int usageNormal = res->getInt("usage_normal");

        delete res;
        delete pstmt;

        if (status == "expired"){
         return false;
        }

        int maxUsage = (accountType == "pro") ? 4 : 2;
        int currentUsage = (accountType == "pro") ? usagePro : usageNormal;

        if (currentUsage >= maxUsage) {
            PreparedStatement* expire = con->prepareStatement(
                "UPDATE discount SET status = 'expired' WHERE id = ?"
            );
            expire->setInt(1, discountId);
            expire->executeUpdate();
            delete expire;
            return false;
        }

        

        if (accountType == "pro") {
            PreparedStatement* update = con->prepareStatement(
                "UPDATE discount SET usage_pro = usage_pro + 1 WHERE id = ?"
            );
            update->setInt(1, discountId);
            update->executeUpdate();
            delete update;
        } else {
            PreparedStatement* update = con->prepareStatement(
                "UPDATE discount SET usage_normal = usage_normal + 1 WHERE id = ?"
            );
            update->setInt(1, discountId);
            update->executeUpdate();
            delete update;
        }

        return true;

    } catch (sql::SQLException &e) {
        cerr << "SQL Error (applyDiscountCode): " << e.what() << endl;
        return false;
    }
     
}
void allCartItems(const string& tableName,string discount ,int mainId , int& items,int &allPrice)
{
    char choise = 'y';
    string discountCode;
    int finalPrice = 0,finalPriceWithDiscountCode = 0;
    bool hadCode;
    deleteZeroProductQuantityFromCard(tableName);
    cout << "Your all cart items:" << endl;

    string show = "SELECT id, quantity, color, price, final_price, product_name AS name FROM " + tableName;
    PreparedStatement* showAll = con->prepareStatement(show);
    ResultSet *rSet = showAll->executeQuery();

    cout << left << setw(10) << "ID" << setw(20) << "Name" << setw(15) << "Price" << setw(12) << "Quantity" << setw(10) << "Color" << setw(10) << "final price" << endl;
    cout << string(70, '-') << endl;

    while(rSet->next()) {
        items += rSet->getInt("quantity");
        cout << left << setw(10) << rSet->getInt("id") << setw(20) << rSet->getString("name") << setw(15) << fixed << rSet->getInt("price") << setw(12) << rSet->getInt("quantity") << setw(10) << rSet->getString("color") << setw(10) << rSet->getInt("final_price") << endl;
        finalPrice += rSet->getInt("final_price");
    }
    
    PreparedStatement* getAccount = con->prepareStatement("SELECT had_discountCode , accountType FROM users WHERE id = ?");
    getAccount->setInt(1, mainId);
    ResultSet* rS = getAccount->executeQuery();
    if (rS->next())
    {
        hadCode = rS->getBoolean("had_discountCode");
    }
    
    if (items > 3 && hadCode == 0)
    {
        createDiscount(discount);
        PreparedStatement *haveCode =
        con->prepareStatement(
            "UPDATE users SET had_discountCode = 1 WHERE id = ?"
        );
        haveCode->setInt(1, mainId);
        haveCode->executeUpdate();
        delete haveCode;

    }
    if (items > 0 )
    {
        while (true) {
            cout << "do you have discount code (y,n)? ";
            cin >> choise;
            
            
            if (!cin.fail()) {
                break;
            }
            cout << "enter valid option" << endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }
        if (choise == 'y')  
        {
            // finalPriceWithDiscountCode = finalPrice;
            while (true) {  
                cout << "Enter the discount code : ";
                cin >> discountCode;
    
                if (!cin.fail()) break;
    
                cout << "enter valid option" << endl;
                cin.clear();
                cin.ignore(1000, '\n');
            }
    
            string accountType;
            // if (rS->next()) {
                accountType = rS->getString("accountType");
            // }
            delete rS;
            delete getAccount;
            // int FDiscount = finalPrice * 20 / 100;
            // finalPrice -= FDiscount;
            if (verifyDiscountCode(discountCode, accountType)) {
                int discount = finalPrice * 20 / 100;
                finalPrice -= discount;
                allPrice = finalPrice;
                cout << "main final price : " << finalPrice + discount << endl;
                cout << "final price with discount : " << finalPrice << endl;
            } else {
                cout << "Invalid or expired discount code!" << endl;
            }
    
        }else{
            allPrice = finalPrice;
        }  
    }
    
    

    

    delete rSet;
    delete showAll;


}
void deleteOfCard(const string& tableName) {
    int productId;
    cout << "Enter Product ID to delete from cart: ";
    cin >> productId;

    try {
        string selectSQL = "SELECT * FROM " + tableName + " WHERE id = ?";
        PreparedStatement* pstmt = con->prepareStatement(selectSQL);
        pstmt->setInt(1, productId);
        ResultSet* rSet = pstmt->executeQuery();

        if (!rSet->next()) {
            cout << "Product not found in cart." << endl;
            delete rSet;
            delete pstmt;
            return;
        }

        string product_name = rSet->getString("product_name");
        int price = rSet->getInt("price");
        int quantity = rSet->getInt("quantity");
        string color = rSet->getString("color");

        delete rSet;
        delete pstmt;

        PreparedStatement* insert = con->prepareStatement(
            "INSERT INTO product(name, price, quantity, color) VALUES (?, ?, ?, ?)"
        );
        insert->setString(1, product_name);
        insert->setInt(2, price);
        insert->setInt(3, quantity);
        insert->setString(4, color);
        insert->executeUpdate();
        delete insert;

        PreparedStatement* deleteFromCard = con->prepareStatement(
            "DELETE FROM " + tableName + " WHERE id = ?"
        );
        deleteFromCard->setInt(1, productId);
        int rows = deleteFromCard->executeUpdate();
        delete deleteFromCard;

        if (rows > 0)
            cout << "Product removed from cart successfully." << endl;

    } catch (sql::SQLException &e) {
        cerr << "error deleteOfCard: " << e.what() << endl;
    }
}
void clearCart(const string& tableName,int& money)
{
    string sql = "DELETE FROM " + tableName;
    Statement* stmt = con->createStatement();
    stmt->executeUpdate(sql);
    delete stmt;
    // string payment = "UPDATE " + tableName + ""
    PreparedStatement *payment = con->prepareStatement("UPDATE valet SET balance = balance - money WHERE user_id = ?");
    payment->setInt(1,money);
    payment->executeUpdate();
    delete payment;
    cout << endl << "Payment was OK" << endl;
}
void updateProfile(int id)
{
    int choise = 0;
    string username,email,password,confirm;
    string oldName,oldEmail,oldPassword;
    cout << "1.edit username" << endl;
    cout << "2.edit password " << endl;
    cout << "3.edit email" << endl;
    cout << "4.exit" << endl;
    while (true) {
        cout << "Choose an option: ";
        cin >> choise;
        
        if (!cin.fail()) {
            break;
        }
        cout << "enter valid option" << endl;
        cin.clear();
        cin.ignore(1000, '\n');
    }

    PreparedStatement *usersInfo = con->prepareStatement("SELECT * FROM users WHERE id = ?");
    usersInfo->setInt(1,id);
    ResultSet *rSet = usersInfo->executeQuery();

    switch (choise)
    {
        case 1:{
            bool flag = true;
            while (flag)
            {
                while (true) {
                    cout << "enter new name : ";
                    cin >> username;
                    
                    auto [user,pass] = findUser(con,username);
                    if (!user.empty())
                    {
                        cout << "this username used by other user " << endl;
                    }
                    
                    if (!cin.fail()) {
                        break;
                    }
                    cout << "enter valid username " << endl;
                    cin.clear();
                    cin.ignore(1000, '\n');
                }
                if (rSet->next())
                {
                    oldName = rSet->getString("name");
                }
                if (oldName == username)
                {
                    cout << "this username is your old username!" << endl;
                    cout << "please enter a new username" << endl;
                    // break;
                }else{
                    flag = false;
                }
            }
            
        
        PreparedStatement * updateUsername = con->prepareStatement("UPDATE users SET name = ? WHERE id = ?");
        updateUsername->setString(1,username);
        updateUsername->setInt(2,id);
        updateUsername->executeUpdate();
        delete updateUsername;
        cout << "edit was successfuly" << endl;
        break;
        }
    case 2:{
        bool flag = true;
        while (flag)
        {
            while (true) {
    
                cout << "enter new password : ";
                cin >> password;
                cout << "enter confirm password : ";
                cin >> confirm;
                
                if (!cin.fail()) {
                    break;
                }
                cout << "enter password !" << endl;
                cin.clear();
                cin.ignore(1000, '\n');
            }
            if (password != confirm)
            {
                cout << "password was not match with confirm !" << endl;
                // break;
            }
            if (rSet->next())
            {
                oldPassword = rSet->getString("password");
            }
            if (oldPassword == password)
            {
                cout << "this password is your old password!" << endl;
                cout << "please enter a new password" << endl;
                // break;
            }else{
                flag = false;
            }
        }
        
        PreparedStatement * updatePasswors = con->prepareStatement("UPDATE users SET password = ? WHERE id = ?");
        updatePasswors->setString(1,password);
        updatePasswors->setInt(2,id);
        updatePasswors->executeUpdate();
        delete updatePasswors;
        cout << "edit was successfuly" << endl;
        break;
    }
    case 3:{
        bool flag = true;
        while (flag)
        {
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
                    // break;
                }
            }else{
                cout << "email not valid !" << endl;
                break;
            }
    
            if (rSet->next())
            {
                oldEmail = rSet->getString("email");
            }
            if (oldEmail == email)
            {
                cout << "this email is your old email!" << endl;
                cout << "please enter a new email" << endl;
                // break;
            }else{
                flag = false;
            }
        }
        
        PreparedStatement * updateEmail = con->prepareStatement("UPDATE users SET email = ? WHERE id = ?");
        updateEmail->setString(1,email);
        updateEmail->setInt(2,id);
        updateEmail->executeUpdate();
        delete updateEmail;
        cout << "edit was successfuly" << endl;
        break;
    }
    default:
        cout << "Enter valid option" << endl;
        break;
    }
}
void showValet(int id)
{
    int balance = 0;
    PreparedStatement *valets = con->prepareStatement("SELECT balance FROM valet WHERE user_id = ?");
    valets->setInt(1,id);
    ResultSet *balanceResult = valets->executeQuery();
    if (balanceResult->next())
    {
        balance = balanceResult->getInt("balance");
    }
    cout << "-----------------------------------------" << endl;
    cout << "| your valet balance : " << balance << setw(10) << "|" << endl;
    cout << "-----------------------------------------" << endl;

}
void getValet(int id)
{
    int choise = 0 , money = 0;
    cout << endl;
    cout << "1. Amount to add" << endl;
    cout << "2. Deduct wallet balance " << endl;
    cout << "3. valet to valet" << endl;
    cout << "4. exit" << endl;
    while (true) {
        cout << "Choose an option: ";
        cin >> choise;
        
        if (!cin.fail()) {
            break;
        }
        cout << "enter valid option" << endl;
        cin.clear();
        cin.ignore(1000, '\n');
    }

    switch (choise)
    {
    case 1:{
    
        while (true) {
            cout << "Amount to add : " ;
            cin >> money;

            if (!cin.fail()) {
                break;
            }
            cout << "enter valid value" << endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }
        PreparedStatement* add = con->prepareStatement("UPDATE valet SET balance = balance + ? WHERE user_id = ?");
        add->setInt(1,money);
        add->setInt(2,id);
        add->executeUpdate();
        delete add;
        showValet(id);
        break;
    }
    case 2:{
        int nowBalance = 0;
        while (true) {
            cout << "Deduct wallet balance : " ;
            cin >> money;

            if (!cin.fail()) {
                break;
            }
            cout << "enter valid value" << endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }
        PreparedStatement * quantity = con->prepareStatement("SELECT balance FROM valet WHERE user_id = ?");
        quantity->setInt(1,id);
        ResultSet *quantityResult = quantity->executeQuery();
        if (quantityResult->next())
        {
            nowBalance = quantityResult->getInt("balance");
        }
        if (nowBalance < money)
        {
            cout << "You dont have enough balance in your wallet" << endl;
            break;
        }
        
        PreparedStatement * deduct = con->prepareStatement("UPDATE valet SET balance = balance - ? WHERE user_id = ?");
        deduct->setInt(1,money);
        deduct->setInt(2,id);
        deduct->executeUpdate();
        showValet(id);
        delete deduct;
        delete quantity;
        delete quantityResult;
        break;
    }
    case 3:{
        string email;
        int destinationId , balance = 0 ;
        bool flag = true;
        while (flag)
        {
            while (true) {
                cout << "enter the email : " ;
                cin >> email;
    
                if (!cin.fail()) {
                    break;
                }
                cout << "enter valid email" << endl;
                cin.clear();
                cin.ignore(1000, '\n');
            }
            while (true) {
                cout << "enter the id : " ;
                cin >> id;
    
                if (!cin.fail()) {
                    break;
                }
                cout << "enter valid id" << endl;
                cin.clear();
                cin.ignore(1000, '\n');
            }
            while (true) {
                cout << "enter the money : " ;
                cin >> money;
    
                if (!cin.fail()) {
                    break;
                }
                cout << "enter valid money" << endl;
                cin.clear();
                cin.ignore(1000, '\n');
            }
            PreparedStatement * quantity = con->prepareStatement("SELECT balance FROM valet WHERE user_id = ?");
            quantity->setInt(1,id);
            ResultSet *quantityResult = quantity->executeQuery();
            if (quantityResult->next())
            {
                balance = quantityResult->getInt("balance");
            }
            if (balance < money)
            {
                cout << "You dont have enough balance in your wallet" << endl;
            }else{
                PreparedStatement * add = con->prepareStatement("UPDATE valet SET balance = balance + ? WHERE user_id = ?");
                add->setInt(1,money);
                add->setInt(2,destinationId);
                add->executeUpdate();
                PreparedStatement * deduct = con->prepareStatement("UPDATE valet SET balance = balance - ? WHERE user_id = ?");
                deduct->setInt(1,money);
                deduct->setInt(2,id);
                deduct->executeUpdate();
                delete deduct;
                delete add;
            }
        }
        
    }

    default:
        break;
    }

}
int main() {
    int sumOfProducts = 0, choice = -1,money = 0;
    char condition = 'y';
    int subChoice = 0 , items = 0;
    string discount;

    Driver* driver = get_driver_instance();
    con = driver->connect("tcp://127.0.0.1:3306", "root", "MyNewPassword123");
    con->setSchema("shop");
    signUp(con);
    auto [mainId, mainUsername] = login(con);
    if (mainId == -1 || mainUsername.empty()) return 1;
    string TBID = to_string(mainId);
    string tableName = "user_" + TBID + "_table";
    do {
        showValet(mainId);
        cout << endl << "1. All Products and Add to cart" << endl;
        cout << "2. All items in your cart" << endl;
        cout << "3. Edit profile" << endl;
        cout << "4. get Valet" << endl;
        cout << "5. Exit" << endl;
        
        while (true) {
            cout << "Choose an option: ";
            cin >> choice;
            

            if (!cin.fail()) {
                break;
            }

            cout << "enter valid option" << endl;
            cin.clear();
            cin.ignore(1000, '\n');
        }

        switch (choice) {
            case 1:
                deleteZeroProductQuantity();
                sumProduct(sumOfProducts);
                if (sumOfProducts == 0) {
                    PreparedStatement* updateId = con->prepareStatement("ALTER TABLE product AUTO_INCREMENT = 1");
                    updateId->executeUpdate();
                    delete updateId;
                    cout << "Oh sorry! products available is done!" << endl << endl;
                    break;
                }
                cout << "All products:" << endl;
                allProduct();

                cout << endl << "1. Add to cart" << endl;
                cout << "2. Go Home" << endl;
                while (true) {
                    cout << "Choose an option: ";
                    cin >> subChoice;

                    if (!cin.fail()) {
                        break;
                    }
                
                    cout << "enter valid option" << endl;
                    cin.clear();
                    cin.ignore(1000, '\n');
                }
                switch (subChoice) {
                    case 1:

                        creatCart(tableName, mainId);
                        addToCart(tableName);
                        break;
                    case 2:
                        condition = 'y';
                        break;
                    default:
                        cout << "Choose a valid option!" << endl;
                        break;
                }
                break;

            case 2:
                creatCart(tableName, mainId);
                allCartItems(tableName,discount,mainId,items,money);
                if (items == 0)
                {
                    cout << "you dont have item in your cart " << endl << endl;
                    break;
                }
                
                cout << endl << "1. Payment the cart" << endl;
                cout << "2. Delete a product from cart" << endl;
                cout << "3. Go to Home" << endl;
                while (true) {
                    
                    cout << "Choose an option: ";
                    cin >> subChoice;
                    if (!cin.fail()) {
                        break;
                    }
                
                    cout << "enter valid option" << endl;
                    cin.clear();
                    cin.ignore(1000, '\n');
                }
                switch (subChoice) {
                    case 1:
                    clearCart(tableName,money);
                    cout << "Payment done." << endl;
                    if (1 == 1)
                    {
                        string updateTableId = "ALTER TABLE "+ tableName +" AUTO_INCREMENT = 1";
                        PreparedStatement* updateId = con->prepareStatement(updateTableId);
                        updateId->executeUpdate();
                        delete updateId;
                    }
                        
                        break;
                    case 2:
                        deleteOfCard(tableName);
                        break;
                    case 3:
                        condition = 'y';
                        break;
                    default:
                        cout << "Choose a valid option!" << endl;
                        break;
                }
                break;
            case 3:
                updateProfile(mainId);
                break;
            case 4:
                getValet(mainId);
                break;
            case 5:
            condition = 'n';
            break;
            default:
                cout << "Choose a valid option!" << endl;
                break;
        }

    } while (condition == 'y');

    delete con;
    return 0;
}
