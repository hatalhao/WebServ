#include "../../includes/Config.hpp"
#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/tokenizer.hpp"
#include "../../includes/networking.hpp"
#include "../../includes/client.hpp"


static inline bool isValidCgi(std::string extension, std::string path) {
	if (extension.empty() || path.empty())
		return false;
	// if (extension != ".php" && extension != ".py"){
	// 	return false;
	// }
	// if (extension == ".php" && path != "/usr/bin/php")
	// 	return false;
	// if (extension == ".py" && path != "/usr/bin/python3"){
	// 	return false;
	// }
	if (access(path.c_str(), F_OK | X_OK) == -1)
		return false;
	return true;
}

static inline cgi_t	getCgi(std::ifstream& configFile, std::string& line) {
	cgi_t cgi;
	size_t pos = NPOS;
	std::string key;
	std::string value;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
		if (isValidBlock(line) == CGI_TOKEN || isValidBlock(line) == LOCATION_TOKEN || isValidBlock(line) == SERVER_TOKEN)
			break;
		if (!isKeyValue(line, CGI_TOKEN)) {
			throw std::runtime_error("Invalid CGI directive: " + line);
		}
		pos = line.find('=');
		if (pos == NPOS) {
			throw std::runtime_error("Invalid CGI directive: " + line);
		}
		
		key = line.substr(0, pos - (line[pos - 1] == ' '));
		removeInvalidChars(key);

		if (key == "extension" && cgi.extension.empty())
			cgi.extension = line.substr(pos + 1 + (line[pos + 1] == ' '));
		else if (key == "handler" && cgi.path.empty())
			cgi.path = line.substr(pos + 1 + (line[pos + 1] == ' '));
		else {
			throw std::runtime_error("Unknown/Duplicate CGI directive: " + key);
		}
		
		value = line.substr(pos + 1 + (line[pos + 1] == ' '));
		if (!isValidValue(value)) {
			throw std::runtime_error("Invalid CGI value: " + line);
		}
		removeInvalidChars(value);
		if (key == "extension")
			cgi.extension = value;
		else if (key == "handler")
			cgi.path = value;
	}
	if (!isValidCgi(cgi.extension, cgi.path)) {
		throw std::runtime_error("Invalid CGI configuration: " + cgi.extension + " -> " + cgi.path);
	}
	return cgi;
}

static inline int	getStatusCode(const std::string& line) {
	long statusCode = 0;
	size_t pos = NPOS;

	pos = line.find_first_not_of("0123456789");
	if (pos != 3 && (line[pos] != '=' || line[pos] != ' ')) {
		return -1;
	}
	statusCode = std::strtol(line.substr(0, pos).c_str(), 0, 10);
	return (isValidStatusCode(statusCode));
}

static inline std::map<int, std::string> getPages(std::ifstream& configFile, std::string& line, std::map<int, std::string>& existingMap) {
	int token = 5;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
		size_t pos = line.find('=');
		token = isValidBlock(line);
		if (pos == NPOS && (token == ERROR_TOKEN || token == LOCATION_TOKEN || token == SERVER_TOKEN || token == CGI_TOKEN)) {
			break ;
		}

		if (pos == NPOS && token == UNKNOWN_TOKEN) {
			throw std::runtime_error("Invalid error page line: " + line);
		}
		int statusCode = getStatusCode(line.substr(line.find_first_not_of(" \t"), pos));
		if (statusCode == -1) {
			throw std::runtime_error("Invalid status code in error page directive: " + line);
		}
		std::string filePath = line.substr(pos + 1);
		if (isValidValue(filePath) == false) {
			throw std::runtime_error("Invalid file path in error page directive: " + line);
		}
		removeInvalidChars(filePath);
		existingMap[statusCode] = filePath;
	}
	return existingMap;
}

static inline std::vector<std::string> extractArray(const std::string& value, bool context) {
	std::vector<std::string> array;
	std::string splitValue = saveArray(value);
	stringstream ss(splitValue);
	splitValue.clear();
	while (std::getline(ss, splitValue, ',')) {
		removeInvalidChars(splitValue);
		if (context && !isValidMethod(splitValue))
			throw std::runtime_error("Invalid method in allow directive: " + splitValue);
		array.push_back(splitValue);
	}
	return array;
}

static inline bool	setValue(const std::string& key, const std::string& value, location_b& location) {
	if (key == "path" && location.path.empty()) {
		location.path = value;
		return true;
	}
	else if (key == "name" && location.name.empty()) {
		location.name = value;
		return true;
	}
	else if (key == "autoindex") {
		if (value == "on") {
			location.isAutoIndexEnabled = true;
			return true;
		}
		else if (value == "off") {
			location.isAutoIndexEnabled = false;
			return true;
		}
		else {
			throw std::runtime_error("Invalid autoindex value: " + value);
		}
	}
	else if (key == "upload_path" && location.uploadPath.empty()) {
		location.uploadPath = value;
		return true;
	}
	else if (key == "return" && location.redirection.empty()) {
		location.redirection = value;
		return true;
	}
	return false;
}

static inline location_b	getLocation(std::ifstream& configFile, std::string& line, const std::string& root, bool autoIndexed) {
	location_b location;
	location.errorPageMap = getDefaultErrorPages();
	location.isAutoIndexEnabled = autoIndexed;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue ;
		size_t pos = line.find('=');
		int token = isValidBlock(line);
		if (pos == NPOS && token == UNKNOWN_TOKEN) {
			throw std::runtime_error("Invalid location line: " + line);
		}

		if (pos == NPOS && (token == LOCATION_TOKEN || token == SERVER_TOKEN)) {
			break ;
		}
		
		if (pos == NPOS && token == CGI_TOKEN) {
			while (isValidBlock(line) == CGI_TOKEN) {
				cgi_t cgi = getCgi(configFile, line);
 				location.cgiList.push_back(cgi);
			}
			if (isValidBlock(line) == LOCATION_TOKEN \
			|| isValidBlock(line) == SERVER_TOKEN) {
				break ;
			}
			continue ;
		}
		if (pos == NPOS && token == ERROR_TOKEN) {
			location.errorPageMap = getPages(configFile, line, location.errorPageMap);
			if (isValidBlock(line) == LOCATION_TOKEN \
			|| isValidBlock(line) == SERVER_TOKEN) {
				break ;
			}
			continue ;
		}

		if (!isKeyValue(line, LOCATION_TOKEN)) {
			throw std::runtime_error("Invalid location directive: " + line);
		}
		
		if (pos != NPOS && pos > 0) {
			std::string key = line.substr(0, pos - (line[pos - 1] == ' '));
			removeInvalidChars(key);
			std::string value = line.substr(pos + 1 + (line[pos + 1] == ' '));
			if (isArrayKey(key) && isArray(value)) {
				if (key == "allow" && location.methods.empty())
					location.methods = extractArray(value, true);
				else if (key == "index" && location.index.empty())
					location.index = extractArray(value, false);
				else
					throw std::runtime_error("Duplicate array directive: " + line);
			}
			else {
				if (!isArrayKey(key) && !isValidValue(value))
					throw std::runtime_error("Invalid array directive: " + line);

				removeInvalidChars(value);
				if (isKeyValue(line, LOCATION_TOKEN) && !setValue(key, value, location))
					throw std::runtime_error("Duplicate or unknown location directive: " + line);

				else if (isValidBlock(line) == CGI_TOKEN) {
					location.isCgiEnabled = true;
					try {
						location.cgiList.push_back(getCgi(configFile, line));
					} catch (const std::runtime_error& e) {
						throw std::runtime_error("Failed to parse CGI configuration: " + std::string(e.what()));
					}
				}
			}
		}
	}
	if (!isValidLocation(location, root)) {
		throw std::runtime_error("Invalid location configuration: " + location.name + " -> " + location.path);
	}
	return location;
}

Config::Config(const std::string& configFilePath) {
	
	std::ifstream configFile(configFilePath.c_str());
	if (!configFile.is_open()) {
		throw std::runtime_error("Could not open configuration file: " + configFilePath);
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
	int	token = SERVER_TOKEN;
	std::string	lastLine = line;

	while (isValidBlock(lastLine) == SERVER_TOKEN) {
		server serv;
		serv.errorPageMap = getDefaultErrorPages();
		do {
			if (skipLine(line))
				continue;

			pos = line.find('=');
			token = (pos == NPOS) ? isValidBlock(line) : token;
			if (pos == NPOS && token == UNKNOWN_TOKEN) {
				configFile.close();
				throw std::runtime_error("Invalid configuration directive in the server block: " + line);
			}
			if (pos == NPOS && token == ERROR_TOKEN) {
				try {
					serv.errorPageMap = getPages(configFile, line, serv.errorPageMap);
					token = isValidBlock(line);
					lastLine = line;
					if (token == SERVER_TOKEN)
						break;
				} catch (const std::runtime_error& e) {
					configFile.close();
					throw std::runtime_error(std::string(e.what()));
				}
			}
			
			if (pos == NPOS && token == LOCATION_TOKEN) {
				try {
					while (isValidBlock(line) == LOCATION_TOKEN) {
						if (_settings["root"].empty())
							throw std::runtime_error("Root directory not specified before location block");
						
						bool autoIndexed = (_settings.find("autoindex") != _settings.end() && _settings["autoindex"] == "on");	
						location_b location = getLocation(configFile, line, serv.root, autoIndexed);
						
						serv.locations.push_back(location);
					}
				} catch (const std::runtime_error& e) {
					configFile.close();
					throw std::runtime_error(std::string(e.what()));
				}
				if (isValidBlock(line) == SERVER_TOKEN) {
					lastLine = line;
					break;
				}
			}

			if (pos == NPOS && token == CGI_TOKEN) {
				configFile.close();
				throw std::runtime_error("CGI directive found outside of location block: " + line);
			}

			if (pos != NPOS && !isKeyValue(line, SERVER_TOKEN)) {
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
				if (!isArrayKey(key) && !isValidValue(value)) {
					configFile.close();
					throw std::runtime_error("Invalid value for key " + key + ": " + value);
				}
				removeInvalidChars(value);
				_settings[key] = value;
			}
		} while (std::getline(configFile, line) && isValidBlock(line) != 1);
		serv.fillServer(serv, *this);
		this->_servers.insert(this->_servers.end(), serv);
		// serv.printServer(this->_servers.back());
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
		if (!isValidPort(portStr)) {
			throw std::runtime_error("Invalid port number: " + portStr);
		}
		port = std::atoi(portStr.c_str());
		ports.insert(port);
	}
	return ports;
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

size_t	Config::extractMaxBodySize() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("max_body_size");
	if (it == _settings.end())
		throw std::runtime_error("Max body size not found in configuration");
	return parseSize(it->second);
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
		return std::vector<std::string>();
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

bool Config::isAutoIndexed() const {
	std::map<std::string, std::string>::const_iterator it = _settings.find("autoindex");
	if (it == _settings.end())
		return false;
	return (it->second == "on");
}

Config::~Config() {
}
