#include "../../includes/Webserv.hpp"
#include "../../includes/Config.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/identifiers.hpp"
// #include "../../includes/networking.hpp"

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
	serv.maxBodySize = config.extractMaxBodySize();

	if (serv.locations.empty() || !isdefaultLocationFound(&serv)) {
		location_b defaultLocation;
		defaultLocation.path = serv.root;
		defaultLocation.name = "/";
		defaultLocation.methods.push_back("GET");
		defaultLocation.isAutoIndexEnabled = false;
		defaultLocation.isCgiEnabled = false;
		serv.locations.push_back(defaultLocation);
	}
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
		std::cout << "Methods: "; 
		for (size_t j = 0; j < location[i].methods.size(); ++j) {
			std::cout << location[i].methods[j] << " ";
		}
		std::cout << std::endl;
		std::cout << (location[i].cgiList.size() ? "CGI: \n" : "");
		for (size_t j = 0; j < location[i].cgiList.size(); ++j) {
			std::cout << "Extension: " << location[i].cgiList[j].extension
					  << ", Handler: " << location[i].cgiList[j].path << endl;
		}
		std::cout << (!(location[i].redirection.empty()) ? "Redirection: " : "");
		std::cout << location[i].redirection << std::endl;
		print endl;
	}
}

void	printServer(const server &serv) {
	std::cout << "Server Configuration:" << std::endl;
	std::cout << "Host: " << serv.host << std::endl;
	for (set<int>::const_iterator it = serv.port.begin(); it != serv.port.end(); ++it) {
		std::cout << "Port: " << *it << std::endl;
	}
	std::cout << "Root: " << serv.root << std::endl;
	std::cout << "Server Name: ";
	for (size_t i = 0; i < serv.serverName.size(); ++i) {
		std::cout << serv.serverName[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "Max Body Size: " << serv.maxBodySize << " bytes" << std::endl;
	std::cout << "Error Pages:" << std::endl;
	for (std::map<int, std::string>::const_iterator it = serv.errorPageMap.begin(); it != serv.errorPageMap.end(); ++it) {
		std::cout << "  " << it->first << ": " << it->second << std::endl;
	}
	printLocation(serv.locations);
}