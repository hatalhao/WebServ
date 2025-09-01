# pragma once

#include "origin.hpp"
#include "networking.hpp"

typedef enum unit {
	B = 1,
	KB = 1024,
	MB = 1024 * KB,
	GB = 1024 * MB
} unit;

class Client;

class Config {
	private:
		std::string							_configFilePath;
		std::map<std::string, std::string>	_settings;
		size_t								_maxBodySize;
		
	public:
		Config(const std::string &configFilePath);
		~Config();
		
		std::vector<Client>					_clients;
		std::vector<server>					_servers;
		
		void								establishConnection();
		void								acceptConnection(int serverIndex, int socketIndex);
		int									handleClientData(int clientFd, int serverIndex);
		void								listenForEvents();
		
		std::set<int>						extractPorts() const;
		std::string							extractHost() const;
		std::string							extractRoot() const;
		std::vector<std::string>			extractIndex() const;
		std::vector<std::string>			extractServerName() const;
		std::map<std::string, std::string>	extractLocations() const;
		std::map<int, std::string>			extractErrorPageMap() const;
		size_t								extractMaxBodySize() const;

		void printParams() {
			std::map<std::string, std::string>::iterator it = _settings.begin();
			std::map<std::string, std::string>::iterator it1 = _settings.end();
			std::cout << "Configuration file path: " << _configFilePath << std::endl;
			std::cout << "Configuration settings:" << std::endl;
			std::pair<std::string, std::string> _pair;
			while (it != it1) {
				_pair = *it;
				std::cout << _pair.first << "=" << _pair.second << std::endl;
				std::cout << "------------------------" << std::endl;
				it++;
			}
	}
};
