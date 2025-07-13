#ifndef ORIGIN_HPP
#define ORIGIN_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>

#define GET = 0x1
#define POST = 0x2
#define DELETE = 0x4
// #define GET = 0x8

#define DEBUG std::cout << "DEBUG: " << __FILE__ << ":" << __LINE__ << std::endl;
#define NPOS std::string::npos
#define print std::cout <<

#endif