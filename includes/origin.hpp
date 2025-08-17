# pragma once

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

class Config;
class Request;
class Client;
struct server;
typedef struct location location_b;

using namespace std;

#define CRLF "\r\n"
#define CRLF_LENGTH 2
#define BRIGHT_GREEN "\x1b[92m"
#define RESET "\x1b[0m"

#define DEBUG std::cout << "DEBUG: " << __FILE__ << ":" << __LINE__ << std::endl;
#define NPOS std::string::npos
#define print std::cout <<

