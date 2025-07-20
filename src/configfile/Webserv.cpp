#include "../../includes/Webserv.hpp"
#include "../../includes/Config.hpp"

template <typename T>
const std::string	to_string(const T& port) {
	std::ostringstream oss;
	oss << port;
	return oss.str();
}

void	checkAttributes(server &serv) {
	if (serv.host.empty())
		throw std::invalid_argument("Host cannot be empty");
	if (serv.port.size() == 0)
		throw std::invalid_argument("Port cannot be empty");

	for (std::set<int>::iterator it = serv.port.begin(); it != serv.port.end(); ++it) {
		if (*it < 1 || *it > 65535)
			throw std::invalid_argument("Invalid port number: " + to_string(*it));
	}

	if (serv.root.empty())
		throw std::invalid_argument("Root cannot be empty");

	if (serv.maxBodySize == 0)
		throw std::invalid_argument("Max body size cannot be zero");
}

void	fillServer(server &serv, const Config &config) {
	serv.port = config.extractPorts();
	serv.host = config.extractHost();
	serv.root = config.extractRoot();
	serv.serverName = config.extractServerName();
	// printServer(serv);
}

void	printLocation(const std::vector<location_b> &location) {
	for (size_t i = 0; i < location.size(); ++i) {
		std::cout << "Location " << i << ":" << std::endl;
		std::cout << "Path: " << location[i].path << std::endl;
		std::cout << "Name: " << location[i].name << std::endl;
		std::cout << "Index: ";
		for (size_t j = 0; j < location[i].index.size(); ++j) {
			std::cout << location[i].index[j] << " ";
		}
		std::cout << std::endl;
		// std::cout << "Methods: ";
		// print location[0].methods.size() << " methods found" << endl;
		// print "location == " << location.size() << endl;
		std::cout << "Methods: ";
		for (size_t j = 0; j < location[i].methods.size(); ++j) {
			std::cout << location[i].methods[j] << " ";
		}
		std::cout << std::endl;
		std::cout << "Redirection: ";
		for (std::map<bool, std::set<std::string> >::const_iterator it = location[i].redirection.begin(); it != location[i].redirection.end(); ++it)
		{
			if (it->first) {
				for (std::set<std::string>::const_iterator sit = it->second.begin(); sit != it->second.end();) {
					std::cout << *sit << ((++sit != it->second.end()) ? " " : "\n");
				}
			}
		}
	}
}

void	printServer(const server &serv) {
	// std::cout << "Server Configuration:" << std::endl;
	// std::cout << "Host: " << serv.host << std::endl;
	// for (set<int>::const_iterator it = serv.port.begin(); it != serv.port.end(); ++it) {
	// 	std::cout << "Port: " << *it << std::endl;
	// }
	// std::cout << "Root: " << serv.root << std::endl;
	// std::cout << "Server Name: ";
	// for (size_t i = 0; i < serv.serverName.size(); ++i) {
	// 	std::cout << serv.serverName[i] << " ";
	// }
	// std::cout << std::endl;
	// std::cout << "Error Pages:" << std::endl;
	// for (std::map<int, std::string>::const_iterator it = serv.errorPageMap.begin(); it != serv.errorPageMap.end(); ++it) {
	// 	std::cout << "  " << it->first << ": " << it->second << std::endl;
	// }
	std::cout << serv.locations.size() << " locations found" << std::endl;
	for (size_t i = 0; i < serv.locations.size(); ++i) {
		printLocation(serv.locations);
	}
}