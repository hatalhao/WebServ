#include "../includes/origin.hpp"
#include "../includes/Config.hpp"

Config::Config(const std::string& configFilePath) : _configFilePath(configFilePath) {
	std::ifstream configFile(_configFilePath.c_str());
	if (!configFile.is_open()) {
		throw std::runtime_error("Could not open configuration file: " + _configFilePath);
	}

	std::string line;
	while (std::getline(configFile, line)) {
		if (line.empty() || line[0] == '#')
			continue;
		if (line == "[server]")
			;	
		size_t pos = line.find('=');
		if (pos != std::string::npos) {
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			_settings[key] = value;
		}
	}
}

Config::~Config() {}