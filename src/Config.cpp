#include "../includes/Config.hpp"
#include "../includes/identifiers.hpp"
#include "../includes/manipulators.hpp"

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
		if (pos == NPOS && !isValidBlock(line))
			throw std::runtime_error("Invalid configuration block: " + line);
		if (pos != NPOS && !isKeyValue(line))
			throw std::runtime_error("Invalid key-value pair: " + line);
		
		if (pos != NPOS && pos > 0) {
			key = line.substr(0, pos - (line[pos - 1] == ' '));
			removeInvalidChars(key);
			value = line.substr(pos + 1 + (line[pos + 1] == ' '));
			if (isArray(value)){
				_settings[key] = saveArray(value);
				print _settings[key] << std::endl;
				continue;
			}
			removeInvalidChars(value);
			_settings[key] = value;
		}
	}
}

std::set<int> Config::extractPorts() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("listen");
	if (it == _settings.end())
		throw std::runtime_error("Port not found in configuration");
	std::set<int> ports;
	std::istringstream ss(it->second);
	std::string portStr;
	int port = 0;
	while (std::getline(ss, portStr, ',')) {
		port = std::atoi(portStr.c_str());
		ports.insert(port);
	}
	return ports;
}

bool	isValidHost(const std::string& host) {
	std::stringstream ss(host);
	int16_t octet;
	int count = 0;
	while (ss >> octet) {
		if (octet > 255 || octet < 0) {
			return false;
		}
		count++;
		if (ss.peek() == '.') {
			ss.ignore();
		} else if (ss.peek() != EOF) {
			return false;
		}
	}
	return (count == 4 && ss.eof());
}

std::string	Config::extractHost() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("host");
	if (it == _settings.end())
		throw std::runtime_error("Host not found in configuration");
	if (!isValidHost(it->second)) {
		throw std::runtime_error("Invalid host format: " + it->second);
	}
	return it->second;
}

std::string	Config::extractRoot() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("root");
	if (it == _settings.end())
		throw std::runtime_error("Root not found in configuration");
	if (access(it->second.c_str(), F_OK & X_OK & W_OK & R_OK) == -1) {
		throw std::runtime_error("Root directory does not exist: " + it->second);
	}
	return it->second;
}

std::vector<std::string>	Config::extractIndex() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("index");
	if (it == _settings.end())
		throw std::runtime_error("Index not found in configuration");
	std::vector<std::string> indexFiles;
	print "Extracting index files: " << it->second << std::endl;
	std::istringstream ss(it->second);
	std::string file;
	while (std::getline(ss, file, ',')) {
		indexFiles.push_back(file);
	}
	print indexFiles.size() << " index files found." << std::endl;
	return indexFiles;
}

std::vector<std::string> Config::extractServerName() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("server_name");
	std::string name;

	if (it == _settings.end())
		name = "nameless";
	std::vector<std::string> serverNames;
	std::istringstream ss(it->second);
	print it->second << std::endl;
	while (std::getline(ss, name, ',')) {
		serverNames.push_back(name);
	}

	for (size_t i = 0; i < serverNames.size(); ++i) {
		print serverNames[i];
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
	int errorCode = 0;
	size_t pos = NPOS;
	for (std::map<std::string, std::string>::const_iterator it = _settings.begin(); it != _settings.end(); ++it) {
		if (it->first.find("error_page") != NPOS) {
			pos = it->first.find('_');
			if (pos != NPOS) {
				errorCode = atoi(it->first.substr(pos + 1).c_str());
				errorPageMap[errorCode] = it->second;
			}
		}
	}
	return errorPageMap;
}

Config::~Config() {}