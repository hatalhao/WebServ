# pragma once

#include "origin.hpp"
#include "Webserv.hpp"
#include "Session.hpp"

typedef enum unit {
	B = 1,
	KB = 1024,
	MB = 1024 * KB
} unit;

class Client;
class CgiHandler;

class Config {
	private:
		std::map<std::string, std::string>	_settings;
		std::vector<server>					_servers;

	public:
		Config(const std::string &configFilePath);
		~Config();
		
		const std::vector<server>&			getServers() const { return _servers; }
		std::set<int>						extractPorts() const;
		std::string							extractHost() const;
		std::string							extractRoot() const;
		std::vector<std::string>			extractIndex() const;
		std::vector<std::string>			extractServerName() const;
		std::map<std::string, std::string>	extractLocations() const;
		std::map<int, std::string>			extractErrorPageMap() const;
		size_t								extractMaxBodySize() const;
		bool								isAutoIndexed() const;
};
