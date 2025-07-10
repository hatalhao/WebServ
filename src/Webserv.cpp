#include "../includes/Webserv.hpp"

void	checkAttributes(server &serv) {
	if (serv.port < 1 || serv.port > 65535)
		throw std::invalid_argument("Invalid port number");

	if (serv.host.empty())
		throw std::invalid_argument("Host cannot be empty");

	if (serv.root.empty())
		throw std::invalid_argument("Root cannot be empty");

	if (serv.index.empty())
		throw std::invalid_argument("Index cannot be empty");

}

void	fillServer(server &serv, const Config &config) {
	serv.port = config.getPort();
	serv.host = config.getHost();
	serv.root = config.getRoot();
	serv.index = config.getIndex();
	serv.serverName = config.getServerName();

	std::map<std::string, std::string> locations = config.getLocations();

	// Fill errorPageMap and cgiList if needed

}