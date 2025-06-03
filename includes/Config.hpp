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
};

#endif