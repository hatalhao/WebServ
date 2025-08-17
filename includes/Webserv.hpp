# pragma once

#include "origin.hpp"
#include "statuscode.hpp"

typedef struct	cgi
{
	string extension;
	string path;
	cgi() : extension(), path() {};
}				cgi_t;

typedef struct	location {
	std::string								name;
	std::string								path;
	std::vector<std::string>				index;
	std::map<int, std::string>				errorPageMap;
	std::vector<std::string>				methods;
	std::map<bool, std::set<std::string> >	redirection;

	bool									isAutoIndexEnabled;
	bool									isCgiEnabled;

	std::vector<cgi_t> cgiList;

	location() : name(), path(), index(), errorPageMap(), methods(), redirection(),
				isAutoIndexEnabled(false), isCgiEnabled(false), cgiList() {};

}				location_b;

typedef struct	server {
	size_t						maxBodySize;
	std::set<int>				port;
	std::string					host;
	std::string					root;
	std::vector<std::string>	serverName;
	std::vector<location_b>		locations;
	std::map<int, std::string>	errorPageMap;
	server() : maxBodySize(0), port(), host(), root(), serverName(), locations(), errorPageMap() {};

}			 server;
void	checkAttributes(server &serv);
void	printServer(const server &serv);
void	fillServer(server &serv, const Config &config);
void	printLocation(const std::vector<location_b> &location);
void	initialiseServer(Config& config);