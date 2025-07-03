#include "../includes/origin.hpp"
#include "../includes/Config.hpp"
#include "../includes/Webserv.hpp"

Config::Config(const std::string& configFilePath) : _configFilePath(configFilePath) {
	std::ifstream configFile(_configFilePath.c_str());
	if (!configFile.is_open()) {
		throw std::runtime_error("Could not open configuration file: " + _configFilePath);
	}

	std::string line;
	int i = 0;
	std::string serie;
	while (std::getline(configFile, line)) {
		if (line.empty() || line[0] == '#')
			continue;
		if (line == "[server]") {
			serie = i + 48;
			Webserv("server " + serie);
			i++;
		}
		size_t pos = line.find('=');
		if (pos != std::string::npos) {
			std::string key = line.substr(0, pos - (line[pos - 1] == ' '));
			std::string value = line.substr(pos + 1 + (line[pos + 1] == ' '));
			_settings[key] = value;
		}
	}
}

u_int16_t Config::getPort() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("listen");
	if (it == _settings.end())
		throw std::runtime_error("Port not found in configuration");
	return (atoi(it->second.c_str()));
}

std::string Config::getHost() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("host");
	if (it == _settings.end())
		throw std::runtime_error("Host not found in configuration");
	return it->second;
}

std::string Config::getRoot() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("root");
	if (it == _settings.end())
		throw std::runtime_error("Root not found in configuration");
	return it->second;
}

std::string Config::getIndex() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("index");
	if (it == _settings.end())
		throw std::runtime_error("Index not found in configuration");
	return it->second;
}

std::string Config::getServerName() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("server_name");
	if (it != _settings.end())
		return it->second;
	return "default_server";
}

std::map<std::string, std::string> Config::getLocations() const {
	std::map<std::string, std::string> locations;
	for (std::map<std::string, std::string>::const_iterator it = _settings.begin(); it != _settings.end(); ++it) {
		if (it->first.find("location") != std::string::npos) {
			locations[it->first] = it->second;
		}
	}
	return locations;
}

std::map<int, std::string>			Config::getErrorPageMap() const {
	std::map<int, std::string> errorPageMap;
	for (std::map<std::string, std::string>::const_iterator it = _settings.begin(); it != _settings.end(); ++it) {
		if (it->first.find("error_page") != std::string::npos) {
			size_t pos = it->first.find('_');
			if (pos != std::string::npos) {
				int errorCode = atoi(it->first.substr(pos + 1).c_str());
				errorPageMap[errorCode] = it->second;
			}
		}
	}
	return errorPageMap;
}

Config::~Config() {}