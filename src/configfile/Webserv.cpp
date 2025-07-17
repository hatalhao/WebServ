#include "../../includes/Webserv.hpp"
#include "../../includes/Config.hpp"

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
	printServer(serv);
}

void	printServer(const server &serv) {
	std::cout << "Server Configuration:" << std::endl;
	std::cout << "Host: " << serv.host << std::endl;
	for (set<int>::const_iterator it = serv.port.begin(); it != serv.port.end(); ++it) {
		std::cout << "Port: " << *it << std::endl;
	}
	std::cout << "Root: " << serv.root << std::endl;
	std::cout << "Index: ";
	for (size_t i = 0; i < serv.index.size(); ++i) {
		std::cout << serv.index[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "Server Name: ";
	for (size_t i = 0; i < serv.serverName.size(); ++i) {
		std::cout << serv.serverName[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "Error Pages:" << std::endl;
	for (std::map<int, std::string>::const_iterator it = serv.errorPageMap.begin(); it != serv.errorPageMap.end(); ++it) {
		std::cout << "  " << it->first << ": " << it->second << std::endl;
	}
}