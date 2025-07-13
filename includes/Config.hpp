#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "origin.hpp"
struct server;

class Config {
	private:
		std::string _configFilePath;
		std::map<std::string, std::string> _settings;
		std::vector<server> _servers;

	public:
		Config(const std::string &configFilePath);
		~Config();

		//getters
		std::set<u_int16_t>					extractPorts() const;
		std::string							extractHost() const;
		std::string							extractRoot() const;
		std::vector<std::string>			extractIndex() const;
		std::vector<std::string>			extractServerName() const;
		std::map<std::string, std::string>	extractLocations() const;
		std::map<int, std::string>			extractErrorPageMap() const;
	

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

#endif