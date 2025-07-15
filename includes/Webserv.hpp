# pragma once

// #include "Config.hpp"
#include "origin.hpp"

typedef struct	cgi
{
	std::string extension;
	std::string path;
}				cgi_t;

typedef struct	location {
	std::string								path;
	std::string								root;
	std::vector<std::string>				index;
	std::map<int, std::string>				errorPageMap;
	std::vector<std::string>				methods;
	std::map<bool, std::set<std::string> >	redirection;

	bool									isAutoIndexEnabled;
	bool									isCgiEnabled;

	std::vector<cgi_t> cgiList;

}				location_b;

typedef struct	server {
	std::set<int>				port;
	std::string					host;
	std::string					root;
	std::vector<std::string>	serverName;
	std::vector<std::string>	index;
	std::vector<location_b>		locations;
	std::map<int, std::string>	errorPageMap;
	
}			 server;
void	checkAttributes(server &serv);
void	printServer(const server &serv);
void	fillServer(server &serv, const Config &config);

