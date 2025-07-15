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
using namespace std;

typedef struct	cgi
{
	string extension;
	string path;
}				cgi_t;

typedef struct	location {
	string path;
	vector<string> methods;
	string root;
	vector<string> index;
	map<int, string> errorPageMap;
	vector<cgi_t> cgiList;
	
	bool isAutoIndexEnabled;
	bool isCgiEnabled;

}				location_b;

typedef struct	server {
	int			port;
	string	host;
	string	root;
	vector<string>	serverName;
	vector<string>	index;
	vector<location_b> locations;
	map<int, string> errorPageMap;
	
	vector<cgi_t> cgiList;

}			 Server;


#endif