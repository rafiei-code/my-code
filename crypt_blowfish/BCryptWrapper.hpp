#ifndef BCRYPT_WRAPPER_HPP
#define BCRYPT_WRAPPER_HPP

#include <string>

extern "C" {
    #include "crypt_blowfish/crypt_blowfish.h"
    #include "crypt_blowfish/crypt_gensalt.h"
    #include "crypt_blowfish/wrapper.h"
}

class BCrypt {
public:
    static std::string generateHash(const std::string& password, int logRounds = 12) {
        char salt[64];
        char hash[128];
        bcrypt_gensalt(logRounds, salt);
        bcrypt_hashpw(password.c_str(), salt, hash);
        return std::string(hash);
    }

    static bool validatePassword(const std::string& password, const std::string& hash) {
        return bcrypt_checkpw(password.c_str(), hash.c_str()) == 0;
    }
};

#endif
