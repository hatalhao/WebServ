#ifndef ORIGIN_HPP
#define ORIGIN_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>

// Forward declarations
class Client;

typedef enum methods {
    GET,
    POST,
    DELETE,
    UNKOWN
} methods;

using namespace std;

#define CRLF "\r\n"

#define DEBUG std::cout << "DEBUG: " << __FILE__ << ":" << __LINE__ << std::endl;
#define NPOS std::string::npos
#define print std::cout <<

#endif