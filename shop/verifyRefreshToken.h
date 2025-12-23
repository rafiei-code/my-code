#pragma once
#include "mainHeader.h"

inline bool verifyRefreshToken(string& username,const string& token)
{
    const string jwtSecretRefresh = "MY_REFRESH_SECRET";
    try {
        auto decoded = jwt::decode(token);
        jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{jwtSecretRefresh})
        .with_issuer("auth_service")
        .verify(decoded);
        
        username = decoded.get_payload_claim("username").as_string();  
        return true; 
    } catch(...) {
        return false;
    }
    
}
