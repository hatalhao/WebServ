#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "origin.hpp"
#include "Config.hpp"
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
	
	// Additional attributes for location
	bool isAutoIndexEnabled;
	bool isCgiEnabled;

}				location_b;

typedef struct	server {
	std::set<int>				port;
	std::string					host;
	std::string					root;
	std::vector<std::string>	serverName;
	std::vector<std::string>	index;
	std::vector<location_b>		locations;
	std::map<int, std::string>	errorPageMap;
	
	std::vector<cgi_t> cgiList;

}			 server;
void	checkAttributes(server &serv);
void	printServer(const server &serv);
void	fillServer(server &serv, const Config &config);


#endif