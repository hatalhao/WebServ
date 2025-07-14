#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdlib>

#define CRLF "\r\n"
typedef struct	cgi
{
	std::string extension;
	std::string path;
}				cgi_t;

typedef struct	location {
	std::string path;
	std::vector<std::string> methods;
	std::string root;
	std::vector<std::string> index;
	std::map<int, std::string> errorPageMap;
	std::vector<cgi_t> cgiList;
	
	bool isAutoIndexEnabled;
	bool isCgiEnabled;

}				location_b;

typedef struct	server {
	int			port;
	std::string	host;
	std::string	root;
	std::vector<std::string>	serverName;
	std::vector<std::string>	index;
	std::vector<location_b> locations;
	std::map<int, std::string> errorPageMap;
	
	std::vector<cgi_t> cgiList;

}			 Server;


#endif