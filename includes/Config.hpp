#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "origin.hpp"
class Webserv;

class Config {
	private:
		std::string _configFilePath;
		std::map<std::string, std::string> _settings;
		std::vector<server> _servers;

	public:
		Config(const std::string &configFilePath);
		~Config();

		//getters
		u_int16_t							getPort() const;
		std::string							getHost() const;
		std::string							getRoot() const;
		std::vector<std::string>			getIndex() const;
		std::vector<std::string>							getServerName() const;
		std::map<std::string, std::string>	getLocations() const;
		std::map<int, std::string>			getErrorPageMap() const;
	
		
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