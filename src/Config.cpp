#include "../includes/origin.hpp"
#include "../includes/Config.hpp"
#include "../includes/Webserv.hpp"

bool notValidChar(char c) {
	return (c == '\"' || c == '\'' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '[' || c == ']');
}

bool trimmableChar(char c) {
	return (c == ' ' || c == '\t' || c == '[' || c == ']');
}

void removeInvalidChars(std::string &str) {
	str.erase(std::remove_if(str.begin(), str.end(), notValidChar), str.end());
}

bool isValidBlock(const std::string& line) {
	if (line.find("=") != NPOS || std::count(line.begin(), line.end(), '[') != 2 || \
	std::count(line.begin(), line.end(), ']') != 2 || line.find("[[") == NPOS \
	|| line.find("]]") == NPOS || line.find("[[") >= line.find("]]")) {
		return false;
	}
	std::string directive = line;
	directive.erase(std::remove_if(directive.begin(), directive.end(), trimmableChar), directive.end());
	return (directive == "error_pages" || directive == "server" || directive == "location" || directive == "cgi");
}

bool skipLine(const std::string& line) {
	size_t pos = line.find_first_not_of(" \t\n\r");
	return (pos == NPOS || line[pos] == '#');
}

bool	isKeyValue(const std::string& line) {
	std::string trimmedLine = line;
	removeInvalidChars(trimmedLine);
	size_t pos = trimmedLine.find('=');
	return (pos != NPOS && pos > 0 && pos < trimmedLine.length() - 1);
}

void	minimumRequirement(std::string &line, std::ifstream& configFile) {
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
		if (!isValidBlock(line))
			throw std::runtime_error("Invalid configuration file");
		removeInvalidChars(line);
		if (line != "server")
			throw std::runtime_error("Invalid configuration file");
		break ;
	}
}

Config::Config(const std::string& configFilePath) : _configFilePath(configFilePath) {
	std::ifstream configFile(_configFilePath.c_str());
	if (!configFile.is_open()) {
		throw std::runtime_error("Could not open configuration file: " + _configFilePath);
	}

	std::string line;
	size_t pos = NPOS;
	std::string key;
	std::string value;
	try {
		minimumRequirement(line, configFile);
	} catch (const std::runtime_error &e) {
		std::cout << "Error: " << e.what() << std::endl;
	}
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
			
		pos = line.find('=');
		if (pos == NPOS && isValidBlock(line) == false)
			throw std::runtime_error("Invalid configuration block: " + line);
		if (pos != NPOS && isKeyValue(line) == false)
			throw std::runtime_error("Invalid key-value pair: " + line);
		
		if (pos != NPOS && pos > 0) {
			key = line.substr(0, pos - (line[pos - 1] == ' '));
			removeInvalidChars(key);
			value = line.substr(pos + 1 + (line[pos + 1] == ' '));
			removeInvalidChars(value);
			_settings[key] = value;
		}
	}
}

std::set<u_int16_t> Config::extractPorts() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("listen");
	if (it == _settings.end())
		throw std::runtime_error("Port not found in configuration");
	std::set<u_int16_t> ports;
	std::istringstream ss(it->second);
	std::string portStr;
	while (ss >> portStr) {
		u_int16_t port = static_cast<u_int16_t>(std::atoi(portStr.c_str()));
		if (port == 0 || port > 65535) {
			throw std::runtime_error("Invalid port number: " + portStr);
		}
		ports.insert(port);
	}
	return ports;
}

std::string Config::extractHost() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("host");
	if (it == _settings.end())
		throw std::runtime_error("Host not found in configuration");
	return it->second;
}

std::string Config::extractRoot() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("root");
	if (it == _settings.end())
		throw std::runtime_error("Root not found in configuration");
	return it->second;
}

std::vector<std::string> Config::extractIndex() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("index");
	if (it == _settings.end())
		throw std::runtime_error("Index not found in configuration");
	std::vector<std::string> indexFiles;
	std::istringstream ss(it->second);
	std::string file;
	while (ss >> file) {
		indexFiles.push_back(file);
	}
	return indexFiles;
}

std::vector<std::string> Config::extractServerName() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("server_name");
	std::string name;

	if (it == _settings.end())
		name = "nameless";
	std::vector<std::string> serverNames;
	std::istringstream ss(it->second);
	while (ss >> name) {
		serverNames.push_back(name);
	}
	return serverNames;
}

std::map<std::string, std::string> Config::extractLocations() const {
	std::map<std::string, std::string> locations;
	for (std::map<std::string, std::string>::const_iterator it = _settings.begin(); it != _settings.end(); ++it) {
		if (it->first.find("location") != NPOS) {
			locations[it->first] = it->second;
		}
	}
	return locations;
}

std::map<int, std::string>	Config::extractErrorPageMap() const {
	std::map<int, std::string> errorPageMap;
	for (std::map<std::string, std::string>::const_iterator it = _settings.begin(); it != _settings.end(); ++it) {
		if (it->first.find("error_page") != NPOS) {
			size_t pos = it->first.find('_');
			if (pos != NPOS) {
				int errorCode = atoi(it->first.substr(pos + 1).c_str());
				errorPageMap[errorCode] = it->second;
			}
		}
	}
	return errorPageMap;
}

Config::~Config() {}