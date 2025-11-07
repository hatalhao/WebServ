#include "../../includes/Webserv.hpp"
#include "../../includes/Config.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/identifiers.hpp"

std::map<int, std::string> getDefaultErrorPages() {
	std::map<int, std::string> defaultErrorPages;
	defaultErrorPages[400] = "./www/html/errors/400.html";
	defaultErrorPages[401] = "./www/html/errors/401.html";
	defaultErrorPages[403] = "./www/html/errors/403.html";
	defaultErrorPages[404] = "./www/html/errors/404.html";
	defaultErrorPages[405] = "./www/html/errors/405.html";
	defaultErrorPages[408] = "./www/html/errors/408.html";
	defaultErrorPages[500] = "./www/html/errors/500.html";
	defaultErrorPages[501] = "./www/html/errors/501.html";
	defaultErrorPages[502] = "./www/html/errors/502.html";
	defaultErrorPages[504] = "./www/html/errors/504.html";
	defaultErrorPages[508] = "./www/html/errors/508.html";
	return defaultErrorPages;
}

static void checkForDuplicateLocations(const std::vector<location_b> &locations) {
	std::map<std::string, std::string> checked;
	for (std::vector<location_b>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
		if (checked.find(it->name) != checked.end() && checked[it->name] == it->path) {
			throw std::runtime_error("Duplicate location found: " + it->name);
		}
		checked[it->name] = it->path;
	}
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
	checkForDuplicateLocations(serv.locations);
}
