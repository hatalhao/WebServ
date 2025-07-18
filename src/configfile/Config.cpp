#include "../../includes/Config.hpp"
#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/tokenizer.hpp"

cgi_t	getCgi(std::ifstream& configFile, std::string& line) {
	cgi_t cgi;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
		if (isValidBlock(line) == CGI_TOKEN)
			break;
		if (!isKeyValue(line)) {
			throw std::runtime_error("Invalid CGI directive: " + line);
		}
	}
	size_t pos = line.find('=');
	if (pos == NPOS) {
		throw std::runtime_error("Invalid CGI directive: " + line);
	}
	cgi.extension = line.substr(0, pos);
	removeInvalidChars(cgi.extension);
	cgi.path = line.substr(pos + 1);
	removeInvalidChars(cgi.path);
	return cgi;
}

location_b	getLocation(std::ifstream& configFile, std::string& line) {
	location_b location;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue ;
		size_t pos = line.find('=');
		if (pos == NPOS && isValidBlock(line) == UNKNOWN_TOKEN) {
			throw std::runtime_error("Invalid location line: " + line);
		}

		if (pos == NPOS && isValidBlock(line) == LOCATION_TOKEN) {
			break ;
		}

		if (pos == NPOS && isValidBlock(line) == CGI_TOKEN) {
			location.cgiList.push_back(getCgi(configFile, line));
			continue ;
		}
		if (!isKeyValue(line)) {
			throw std::runtime_error("Invalid location directive: " + line);
		}
		if (pos != NPOS && pos > 0) {
			std::string key = line.substr(0, pos - (line[pos - 1] == ' '));
			removeInvalidChars(key);
			std::string value = line.substr(pos + 1 + (line[pos + 1] == ' '));
			if (isArrayKey(key) && isArray(value)) {
				if (key == "methods") {
					std::vector<std::string> method;
					size_t arrayPos = value.find(',');
					while (arrayPos != NPOS) {
						method.push_back(value.substr(arrayPos * (method.size() > 0), arrayPos));
					}
				}
			}
		}
	}
	return location;
}

std::map<int, std::string> getPages(std::ifstream& configFile) {
	std::map<int, std::string> errorPageMap;
	std::string line;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
		size_t pos = line.find('=');

		if (pos == NPOS && isValidBlock(line) == ERROR_TOKEN) {
			break ;
		}

		if (pos == NPOS && isValidBlock(line) == UNKNOWN_TOKEN) {
			throw std::runtime_error("Invalid error page line: " + line);
		}

		int statusCode = std::atoi(line.substr(0, pos).c_str());
		std::string filePath = line.substr(pos + 1);
		removeInvalidChars(filePath);
		errorPageMap[statusCode] = filePath;
	}
	return errorPageMap;
}

Config::Config(const std::string& configFilePath) : _configFilePath(configFilePath) {
	std::ifstream configFile(_configFilePath.c_str());
	if (!configFile.is_open()) {
		throw std::runtime_error("Could not open configuration file: " + _configFilePath);
	}

	std::string	line;
	size_t pos = NPOS;
	std::string	key;
	std::string	value;
	try {
		minimumRequirement(line, configFile);
	} catch (const std::runtime_error &e) {
		configFile.close();
		throw std::runtime_error("Invalid configuration file: " + line);
	}
	static int	token = SERVER_TOKEN;
	static std::string	lastLine = line;

	while (lastLine == "[[server]]") {
		server serv;

		do {
			if (skipLine(line))
				continue;
	
			pos = line.find('=');
			token = (pos == NPOS) ? isValidBlock(line) : token;
			if (pos == NPOS && token == UNKNOWN_TOKEN) {
				configFile.close();
				throw std::runtime_error("Invalid configuration directive: " + line);
			}
			if (pos == NPOS && token == ERROR_TOKEN) {
				try {
					serv.errorPageMap = getPages(configFile);
				} catch (const std::runtime_error& e) {
					configFile.close();
					throw std::runtime_error(std::string(e.what()));
				}
			}
			if (NPOS == pos && token == LOCATION_TOKEN) {
				try {
					serv.locations.push_back(getLocation(configFile, line));
				} catch (const std::runtime_error& e) {
					configFile.close();
					throw std::runtime_error(std::string(e.what()));
				}
			}

			if (pos != NPOS && !isKeyValue(line)) {
				configFile.close();
				throw std::runtime_error("Invalid key-value pair: " + line);
			}
			lastLine = line;
			if (pos != NPOS && pos > 0) {
				key = line.substr(0, pos - (line[pos - 1] == ' '));
				removeInvalidChars(key);
				if (_settings.find(key) != _settings.end()) {
					configFile.close();
					throw std::runtime_error("Duplicate key found: " + key);
				}
				value = line.substr(pos + 1 + (line[pos + 1] == ' '));
				if (isArrayKey(key) && isArray(value)) {
					_settings[key] = saveArray(value);
					continue;
				}
				removeInvalidChars(value);
				_settings[key] = value;
			}
		} while (std::getline(configFile, line) && isValidBlock(line) != 1);
		fillServer(serv, *this);
		this->_servers.insert(this->_servers.end(), serv);
		this->_settings.clear();
	}
	configFile.close();
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
	if (access(it->second.c_str(), F_OK | X_OK) == -1) {
		throw std::runtime_error("Root Directory is inaccessible/not found: " + it->second);
	}
	return it->second;
}

std::vector<std::string>	Config::extractIndex() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("index");
	if (it == _settings.end())
		throw std::runtime_error("Index not found in configuration");
	std::vector<std::string> indexFiles;
	std::istringstream ss(it->second);
	std::string file;
	while (std::getline(ss, file, ',')) {
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
	while (std::getline(ss, name, ',')) {
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