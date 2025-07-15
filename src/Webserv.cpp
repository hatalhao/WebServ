#include "../includes/Webserv.hpp"

template <typename T>
const std::string	to_string(const T& port) {
	std::ostringstream oss;
	oss << port;
	return oss.str();
}

void	checkAttributes(server &serv) {
	if (serv.port.size() == 0)
		throw std::invalid_argument("Port cannot be empty");
	for (std::set<int>::iterator it = serv.port.begin(); it != serv.port.end(); ++it) {
		if (*it < 1 || *it > 65535)
			throw std::invalid_argument("Invalid port number: " + to_string(*it));
	}

	if (serv.host.empty())
		throw std::invalid_argument("Host cannot be empty");

	if (serv.root.empty())
		throw std::invalid_argument("Root cannot be empty");

	
}

void	fillServer(server &serv, const Config &config) {
	serv.port = config.extractPorts();
	serv.host = config.extractHost();
	serv.root = config.extractRoot();
	serv.index = config.extractIndex();
	serv.serverName = config.extractServerName();

	// Fill errorPageMap and cgiList if needed

}