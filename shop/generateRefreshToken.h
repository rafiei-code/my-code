#include "mainHeader.h"
string generateRefreshToken(const string& username) {
    const string jwtSecretRefresh = "MY_REFRESH_SECRET";
    return jwt::create()
        .set_issuer("auth_service")
        .set_type("jwt")
        .set_subject("refresh_token")
        .set_payload_claim("username", jwt::claim(username))
        .set_expires_at(chrono::system_clock::now() + chrono::hours(24*10))
        .sign(jwt::algorithm::hs256{jwtSecretRefresh});
}