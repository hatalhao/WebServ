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
	std::string								redirection;
	std::string								uploadPath;

	bool									isAutoIndexEnabled;
	bool									isCgiEnabled;

	std::vector<cgi_t> cgiList;

	location() : name(), path(), index(), errorPageMap(), methods(), redirection(), uploadPath(),
				isAutoIndexEnabled(false), isCgiEnabled(false), cgiList() {};

}				location_b;

typedef struct	server {
	std::vector<std::string>	index;
	size_t						maxBodySize;
	std::set<int>				port;
	std::string					host;
	std::string					root;
	vector<int>					socketFd;
	std::vector<std::string>	serverName;
	std::vector<location_b>		locations;
	std::map<int, std::string>	errorPageMap;
	server() : index(), maxBodySize(0), port(), host(), root(), socketFd(), serverName(), locations(), errorPageMap() {};
	
	
	// Member functions
	void	createSockets(const struct addrinfo& hint, struct addrinfo **newAddr, const std::string& port, const std::string& host);
	void	fileDescriptorDisposal();

}			 server;
void	checkAttributes(server &serv);
void	printServer(const server &serv);
void	fillServer(server &serv, const Config &config);
void	printLocation(const std::vector<location_b> &location);
void	initialiseServer(Config& config);
std::map<int, std::string>	getDefaultErrorPages();