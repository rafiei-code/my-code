#pragma once
#include <string>
#include <cppconn/connection.h>
#include "httplib.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include "BCrypt.hpp"
#include <random>
#include <jwt-cpp/jwt.h>

using namespace std;
using namespace sql;
using namespace httplib;
using json = nlohmann::json;


extern Connection* con;
extern const string jwtSecretAccess;
extern const string jwtSecretRefresh;
