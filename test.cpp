#include <iostream>
#include <random>
using namespace std;
int main()
{
   char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    // random_device rd;
    mt19937 gen(4562);
    uniform_int_distribution<> dis(0, sizeof(chars) - 2);

    string token;
    //  for(int i=0; i < 4 ; ++i)
        token += chars[dis(gen)];
        cout << token << endl;
}