#include "../includes/origin.hpp"
#include "../includes/Config.hpp"
#include "../includes/Webserv.hpp"

bool notValidChar(char c) {
	return (c == '\"' || c == '\'' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '[' || c == ']');
}

bool isValidBlock(const std::string& line) {
	if (line.find("=") != std::string::npos || std::count(line.begin(), line.end(), '[') != 2 || \
	std::count(line.begin(), line.end(), ']') != 2 || line.find("[[") == std::string::npos \
	|| line.find("]]") == std::string::npos || line.find("[[") >= line.find("]]")) {
		return false;
	}
	std::string directive = line;
	directive.erase(std::remove_if(directive.begin(), directive.end(), notValidChar), directive.end());
	return (directive == "error_pages" || directive == "server" || directive == "location" || directive == "cgi");
}

Config::Config(const std::string& configFilePath) : _configFilePath(configFilePath) {
	std::ifstream configFile(_configFilePath.c_str());
	if (!configFile.is_open()) {
		throw std::runtime_error("Could not open configuration file: " + _configFilePath);
	}

	std::string line;
	size_t pos = std::string::npos;
	std::string key;
	std::string value;
	while (std::getline(configFile, line)) {
		if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t\n\r") == std::string::npos)
			continue;
		if (isValidBlock(line) == false && line.find('=') == std::string::npos)
			throw std::runtime_error("Invalid configuration block: " + line);
		pos = line.find('=');
		if (pos != std::string::npos) {
			key = line.substr(0, pos - (line[pos - 1] == ' '));
			key.erase(std::remove_if(key.begin(), key.end(), notValidChar), key.end());
			value = line.substr(pos + 1 + (line[pos + 1] == ' '));
			value.erase(std::remove_if(value.begin(), value.end(), notValidChar), value.end());
			_settings[key] = value;
		}
	}
	this->printParams();
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

std::vector<std::string>* Config::getIndex() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("index");
	if (it == _settings.end())
		throw std::runtime_error("Index not found in configuration");
	std::vector<std::string> *indexFiles = new std::vector<std::string>;
	std::istringstream ss(it->second);
	std::string file;
	while (ss >> file) {
		indexFiles->push_back(file);
	}
	return indexFiles;
}

std::vector<std::string> Config::getServerName() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("server_name");
	if (it == _settings.end())
		throw std::runtime_error("Server name not found in configuration");
	std::vector<std::string> serverNames;
	std::istringstream ss(it->second);
	std::string name;
	while (ss >> name) {
		serverNames.push_back(name);
	}
	return serverNames;
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

std::map<int, std::string>	Config::getErrorPageMap() const {
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