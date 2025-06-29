#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "origin.hpp"

class Config {
	private:
		std::string _configFilePath;
		std::map<std::string, std::string> _settings;

	public:
		Config(const std::string &configFilePath);
		~Config();
		
		
		
		void printParams() {
			std::map<std::string, std::string>::iterator it = _settings.begin();
			std::map<std::string, std::string>::iterator it1 = _settings.end();
			std::cout << "Configuration file path: " << _configFilePath << std::endl;
			std::cout << "Configuration settings:" << std::endl;
			std::pair<std::string, std::string> _pair;
			while (it != it1) {
				_pair = *it;
				std::cout << _pair.first << "=" << _pair.second << std::endl;
				++it;
			}
		}

};

#endif