#include <iostream>
#include <jwt-cpp/jwt.h>
using namespace std;

int main() {
    string secret = "MY_SECRET_KEY";

    auto token = jwt::create()
        .set_issuer("myserver")
        .set_type("JWS")
        .set_payload_claim("userId", jwt::claim(picojson::value((double)42)))
        .set_payload_claim("role", jwt::claim(string("admin")))
        .set_expires_at(
            chrono::system_clock::now() + chrono::minutes(10)
        )
        .sign(jwt::algorithm::hs256{secret});

    cout << token << endl;

    return 0;
}
