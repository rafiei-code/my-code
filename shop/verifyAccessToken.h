#pragma once
#include "mainHeader.h"

inline bool verifyAccessToken(string& username,string& role,const string& token)
{
    try {
        const string jwtSecretAccess = "MY_ACCESS_SECRET";

        auto decoded = jwt::decode(token);
        jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{jwtSecretAccess})
        .with_issuer("auth_service")
        .verify(decoded);
        
        username = decoded.get_payload_claim("username").as_string();
        role = decoded.get_payload_claim("role").as_string();
        return true;
    } catch(...) {
        return false;
    }
    
}