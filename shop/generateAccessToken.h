#include "mainHeader.h"

string generateAccessToken(const string& username,const string& role)
{
    const string jwtSecretAccess = "MY_ACCESS_SECRET";
    return jwt::create()
        .set_issuer("auth_service")
        .set_type("jwt")
        .set_subject("access_token")
        .set_payload_claim("username", jwt::claim(username))
        .set_payload_claim("role",jwt::claim(role))
        .set_expires_at(chrono::system_clock::now()+chrono::hours(1))
        .sign(jwt::algorithm::hs256{jwtSecretAccess});

}