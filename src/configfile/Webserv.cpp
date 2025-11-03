#include "../../includes/Webserv.hpp"
#include "../../includes/Config.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/identifiers.hpp"
// #include "../../includes/networking.hpp"

std::map<int, std::string> getDefaultErrorPages() {
	std::map<int, std::string> defaultErrorPages;
	defaultErrorPages[400] = "./www/html/errors/400.html";
	defaultErrorPages[403] = "./www/html/errors/403.html";
	defaultErrorPages[404] = "./www/html/errors/404.html";
	defaultErrorPages[405] = "./www/html/errors/405.html";
	defaultErrorPages[408] = "./www/html/errors/408.html";
	defaultErrorPages[500] = "./www/html/errors/500.html";
	defaultErrorPages[501] = "./www/html/errors/501.html";
	defaultErrorPages[502] = "./www/html/errors/502.html";
	return defaultErrorPages;
}

void	server::fillServer(server &serv, const Config &config) {
	serv.port = config.extractPorts();
	serv.host = config.extractHost();
	serv.root = config.extractRoot();
	serv.serverName = config.extractServerName();
	serv.maxBodySize = config.extractMaxBodySize();
	serv.index = config.extractIndex();
	if (serv.locations.empty() || !isdefaultLocationFound(serv)) {
		location_b defaultLocation;
		defaultLocation.path = serv.root;
		defaultLocation.name = "/";
		defaultLocation.methods.push_back("GET");
		defaultLocation.errorPageMap = getDefaultErrorPages();
		defaultLocation.isAutoIndexEnabled = true;
		defaultLocation.index = serv.index;
		defaultLocation.isCgiEnabled = false;
		defaultLocation.errorPageMap = serv.errorPageMap;
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
		for (std::map<int, std::string>::const_iterator it = location[i].errorPageMap.begin(); it != location[i].errorPageMap.end(); ++it) {
			std::cout << "  " << it->first << ": " << it->second << std::endl;
		}
		std::cout << "Autoindex: " << (location[i].isAutoIndexEnabled ? "on" : "off") << std::endl;
		print "==================================================================================\n";
	}
}

// void	server::printServer(const server &serv) {
// 	std::cout << "Server Configuration:" << std::endl;
// 	std::cout << "Host: " << serv.host << std::endl;
// 	for (set<int>::const_iterator it = serv.port.begin(); it != serv.port.end(); ++it) {
// 		std::cout << "Port: " << *it << std::endl;
// 	}
// 	std::cout << "Root: " << serv.root << std::endl;
// 	std::cout << "Server Name: ";
// 	for (size_t i = 0; i < serv.serverName.size(); ++i) {
// 		std::cout << serv.serverName[i] << " ";
// 	}
// 	std::cout << std::endl;
// 	std::cout << "Max Body Size: " << serv.maxBodySize << " bytes" << std::endl;
// 	std::cout << "Error Pages:" << std::endl;
// 	for (std::map<int, std::string>::const_iterator it = serv.errorPageMap.begin(); it != serv.errorPageMap.end(); ++it) {
// 		std::cout << "  " << it->first << ": " << it->second << std::endl;
// 	}
// 	printLocation(serv.locations);
// }