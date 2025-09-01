#include "../../includes/Config.hpp"
#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/tokenizer.hpp"
#include "../../includes/networking.hpp"
#include "../../includes/client.hpp"
#include <set>
#include <errno.h>
#include <cstring>

bool isValidCgi(std::pair<std::string, std::string> cgi) {
	if (cgi.first.empty() || cgi.second.empty())
		return false;
	if (cgi.first == ".php" && cgi.second != "/usr/bin/php")
		return false;
	if (cgi.first == ".py" && cgi.second != "/usr/bin/python3"){
		return false;
	}
	if (access(cgi.second.c_str(), F_OK | X_OK) == -1)
		return false;
	return true;
}

cgi_t	getCgi(std::ifstream& configFile, std::string& line) {
	cgi_t cgi;
	size_t pos = NPOS;
	std::string key;
	std::string value;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
		if (isValidBlock(line) == CGI_TOKEN || isValidBlock(line) == LOCATION_TOKEN || isValidBlock(line) == SERVER_TOKEN)
			break;
		if (!isKeyValue(line)) {
			throw std::runtime_error("Invalid CGI directive: " + line);
		}
		pos = line.find('=');
		if (pos == NPOS) {
			throw std::runtime_error("Invalid CGI directive: " + line);
		}
		key = line.substr(0, pos - (line[pos - 1] == ' '));
		removeInvalidChars(key);
		if (key == "extension")
			cgi.extension = line.substr(pos + 1 + (line[pos + 1] == ' '));
		else if (key == "handler")
			cgi.path = line.substr(pos + 1 + (line[pos + 1] == ' '));
		else {
			throw std::runtime_error("Unknown CGI directive: " + key);
		}
		value = line.substr(pos + 1 + (line[pos + 1] == ' '));
		removeInvalidChars(value);

		if (key == "extension" )
			cgi.extension = value;
		else if (key == "handler")
			cgi.path = value;
	}
	if (!isValidCgi(std::make_pair(cgi.extension, cgi.path))) {
		throw std::runtime_error("Invalid CGI configuration: " + cgi.extension + " -> " + cgi.path);
	}
	return cgi;
}

location_b	getLocation(std::ifstream& configFile, std::string& line, const std::string& root) {
	location_b location;
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
			while (line == "[[cgi]]"){
				cgi_t cgi = getCgi(configFile, line);
 				location.cgiList.push_back(cgi);
			}
			if (isValidBlock(line) == LOCATION_TOKEN \
			|| isValidBlock(line) == SERVER_TOKEN) {
				break ;
			}
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
				std::vector<std::string> method;
				std::vector<std::string> redirection;
				if (key == "allow") {
					value = saveArray(value);
					stringstream ss(value);
					value.clear();
					while (std::getline(ss, value, ',')) {
						removeInvalidChars(value);
						if (!isValidMethod(value))
							throw std::runtime_error("Invalid method in allow directive: " + value);
						method.push_back(value);
					}
					location.methods = method;
				}

				if (key == "index") {
					value = saveArray(value);
					stringstream ss(value);
					value.clear();
					while (std::getline(ss, value, ',')) {
						removeInvalidChars(value);
						location.index.push_back(value);
					}
				}
			}
			else {
				removeInvalidChars(value);
				if (key == "return") {location.redirection = value; }
				else if (key == "upload_path") { location.uploadPath = value; }
				else if (key == "path") { location.path = value; }
				else if (key == "name") { location.name = value; }
				else if (key == "autoindex") { location.isAutoIndexEnabled = (value == "on"); }
				else if (key == "error_page") {
					size_t errorPos = value.find('=');
					if (errorPos != NPOS) {
						int errorCode = std::atoi(value.substr(0, errorPos).c_str());
						std::string errorPagePath = value.substr(errorPos + 1);
						removeInvalidChars(errorPagePath);
						location.errorPageMap[errorCode] = errorPagePath;
					}
				} else if (isValidBlock(line) == CGI_TOKEN) {
					location.isCgiEnabled = (value == "on");
					try {
						location.cgiList.push_back(getCgi(configFile, line));
					} catch (const std::runtime_error& e) {
						throw std::runtime_error("Failed to parse CGI configuration: " + std::string(e.what()));
					}
				} else {
					throw std::runtime_error("Unknown location directive: " + key);
				}
			}
		}
	}
	if (!isValidLocation(location, root)) {
		throw std::runtime_error("Invalid location configuration: " + location.name + " -> " + location.path);
	}
	return location;
}

int	getStatusCode(const std::string& line) {
	long statusCode = 0;
	size_t pos = NPOS;

	pos = line.find_first_not_of("0123456789");
	if (pos != 3 && (line[pos] != '=' || line[pos] != ' ')) {
		return -1;
	}
	statusCode = std::strtol(line.substr(0, pos).c_str(), 0, 10);
	return (isValidStatusCode(statusCode));
}

std::map<int, std::string> getPages(std::ifstream& configFile, std::string& line) {
	std::map<int, std::string> errorPageMap;
	int token = 5;
	while (std::getline(configFile, line)) {
		if (skipLine(line))
			continue;
		size_t pos = line.find('=');
		token = isValidBlock(line);
		if (pos == NPOS && (token == ERROR_TOKEN || token == LOCATION_TOKEN || token == SERVER_TOKEN)) {
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
	(void) this->_maxBodySize ;
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
					serv.errorPageMap = getPages(configFile, line);
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
					while (line == "[[location]]") {
						if (_settings["root"].empty())
							throw std::runtime_error("Root directory not specified before location block");
						location_b location = getLocation(configFile, line, serv.root);
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

size_t parseSize(const std::string& sizeStr) {
	if (sizeStr.empty()) {
		throw std::runtime_error("Size string is empty");
	}
	size_t size = 0;
	size_t pos = sizeStr.find_first_not_of("0123456789");
	std::string numberStr = sizeStr.substr(0, pos);
	if (pos != NPOS) {
		std::string unitStr = sizeStr.substr(pos);
		std::transform(unitStr.begin(), unitStr.end(), unitStr.begin(), ::toupper);
		if (unitStr == "B") {
			size = std::atoll(numberStr.c_str());
		} else if (unitStr == "KB") {
			size = std::atoll(numberStr.c_str()) * KB;
		} else if (unitStr == "MB") {
			size = std::atoll(numberStr.c_str()) * MB;
		} else if (unitStr == "GB") {
			size = std::atoll(numberStr.c_str()) * GB;
		} else {
			throw std::runtime_error("Invalid size unit: " + unitStr);
		}
	} else {
		size = std::atoll(sizeStr.c_str());
	}
	return size;
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
			pos = it->first.find('=');
			if (pos != NPOS) {
				errorCode = atoi(it->first.substr(pos - 1).c_str());
				print "Error Code == " << errorCode << endl;
				errorPageMap[errorCode] = it->second;
			}
		}
	}
	return errorPageMap;
}

Config::~Config() {
	// Clean up all client pointers
	for (size_t i = 0; i < this->_clients.size(); ++i) {
		delete this->_clients[i];
	}
	this->_clients.clear();
}

int	Config::acceptConnection(int serverIndex, int socketIndex) {
	if (serverIndex >= static_cast<int>(this->_servers.size()) || 
		socketIndex >= static_cast<int>(this->_servers[serverIndex].socketFd.size())) {
		return -1;
	}
	
	int clientFd = accept(this->_servers[serverIndex].socketFd[socketIndex], NULL, NULL);
	if (clientFd < 0) {
		return -1;
	}

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
		close(clientFd);
		return -1;
	}

	// Create client on heap to avoid stack-use-after-return
	Client* newClient = new Client(clientFd, &this->_servers[serverIndex]);
	this->_clients.push_back(newClient);
	int clientIndex = this->_clients.size() - 1;

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = clientFd;
	if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
		perror("epoll_ctl: clientFd");
		close(clientFd);
		delete this->_clients.back(); // Delete the client we just added
		this->_clients.pop_back();
		return -1;
	}

	print "Accepted new client connection on socket " << clientFd << " (client index: " << clientIndex << ")" << endl;
	return clientFd;
}

// int	Client::handleData() {
// 	char buffer[4096];
// 	print "Receiving data on client socket " << this->_clientFd << endl;
// 	ssize_t bytesRead = recv(this->_clientFd, buffer, sizeof(buffer) - 1, 0);
	
// 	if (bytesRead > 0) {
// 		buffer[bytesRead] = '\0';
// 		string requestData(buffer);
// 		this->setRequestBuf(requestData);
// 		this->processRequest();
// 		if (this->getState() == READING_DONE || this->getState() == READING_BAD_REQUEST) {
// 			print "Received " << requestData.length() << " bytes from client " << this->_clientFd << endl;
// 			print "The Full Request is: " << requestData << endl;
// 			this->generateResponse();
			
// 			string responseBuffer = this->getResponseBuf();
			
// 			size_t totalSent = 0;
// 			size_t toSend = responseBuffer.length();
// 			const char* buf = responseBuffer.c_str();
			
// 			while (totalSent < toSend) {
// 				ssize_t sent = send(this->_clientFd, buf + totalSent, toSend - totalSent, 0);
// 				if (sent <= 0) {
// 					break;
// 				}
// 				totalSent += sent;
// 			}
			
// 			return (FINISHED);
// 		}	
// 	} else if (bytesRead == 0) {
// 		print "Client " << this->_clientFd << " disconnected" << endl;
// 		return (DISCONNECTED);
// 	} else if (bytesRead < 0) {
// 		print "Error reading from client " << this->_clientFd << ": " << strerror(errno) << endl;
// 		return (DISCONNECTED);
// 	}
// 	return (CONTINUE);
// }

void Config::listenForEvents() {
    print "Starting epoll-based event loop" << endl;

    if (this->_servers.empty()) {
        throw std::runtime_error("No servers configured");
    }

    this->_epollFd = epoll_create1(0);
    if (this->_epollFd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }

    std::map<int, std::pair<int, int> > socketToServer;  // serverSocket → (serverIndex, socketFdIndex)
    std::map<int, int> clientToServer;                   // clientFd → serverIndex
    std::map<int, int> fdToClientIndex;                  // clientFd → client index in _clients

    // Register server sockets
    for (size_t serverIndex = 0; serverIndex < this->_servers.size(); ++serverIndex) {
        for (size_t socketFdIndex = 0; socketFdIndex < this->_servers[serverIndex].socketFd.size(); ++socketFdIndex) {
            int serverSocket = this->_servers[serverIndex].socketFd[socketFdIndex];

            if (fcntl(serverSocket, F_SETFL, O_NONBLOCK) < 0) {
                close(this->_epollFd);
                throw std::runtime_error("Failed to set server socket non-blocking");
            }

            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.fd = serverSocket;

            if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, serverSocket, &event) < 0) {
                close(this->_epollFd);
                throw std::runtime_error("Failed to add server socket to epoll");
            }

            socketToServer[serverSocket] = std::make_pair(serverIndex, socketFdIndex);
        }
    }

    struct epoll_event events[MAXEVENTS];

    while (true) {
        int eventCount = epoll_wait(this->_epollFd, events, MAXEVENTS, -1);
        if (eventCount < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < eventCount; ++i) {
            int fd = events[i].data.fd;

            // New incoming connection?
            if (socketToServer.find(fd) != socketToServer.end()) {
                std::pair<int, int> serverInfo = socketToServer[fd];
                int serverIndex = serverInfo.first;
                int socketIndex = serverInfo.second;

                int clientFd = this->acceptConnection(serverIndex, socketIndex);
                if (clientFd >= 0) {
                    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
                        print "Failed to set client socket non-blocking" << endl;
                        close(clientFd);
                        continue;
                    }

                    struct epoll_event clientEvent;
                    clientEvent.events = EPOLLIN | EPOLLET; // start with read
                    clientEvent.data.fd = clientFd;

                    if (epoll_ctl(this->_epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) < 0) {
                        if (errno == EEXIST) {
                            if (epoll_ctl(this->_epollFd, EPOLL_CTL_MOD, clientFd, &clientEvent) < 0) {
                                print "Failed to modify client socket in epoll: " << strerror(errno) << endl;
                                close(clientFd);
                                continue;
                            }
                        } else {
                            print "Failed to add client socket to epoll: " << strerror(errno) << endl;
                            close(clientFd);
                            continue;
                        }
                    }

                    clientToServer[clientFd] = serverIndex;
                    fdToClientIndex[clientFd] = this->_clients.size() - 1;
                }
            }
            // Existing client activity
            else {
                std::map<int, int>::iterator clientIt = fdToClientIndex.find(fd);
                if (clientIt != fdToClientIndex.end()) {
                    int clientIndex = clientIt->second;
                    Client* client = this->_clients[clientIndex];

                    // Error or hangup → close client
                    if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                        epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        clientToServer.erase(fd);
                        fdToClientIndex.erase(clientIt);

                        delete this->_clients[clientIndex];
                        this->_clients.erase(this->_clients.begin() + clientIndex);

                        for (std::map<int, int>::iterator mapIt = fdToClientIndex.begin();
                             mapIt != fdToClientIndex.end(); ++mapIt) {
                            if (mapIt->second > clientIndex) {
                                mapIt->second--;
                            }
                        }
                        print "Closed connection to client " << fd << endl;
                        continue;
                    }

                    // Handle readable client
                    if (events[i].events & EPOLLIN) {
                        int result = client->handleReadEvent(this->_epollFd);
                        if (result == DISCONNECTED || result == FINISHED) {
                            epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
                            close(fd);
                            clientToServer.erase(fd);
                            fdToClientIndex.erase(clientIt);

                            delete this->_clients[clientIndex];
                            this->_clients.erase(this->_clients.begin() + clientIndex);

                            for (std::map<int, int>::iterator mapIt = fdToClientIndex.begin();
                                 mapIt != fdToClientIndex.end(); ++mapIt) {
                                if (mapIt->second > clientIndex) {
                                    mapIt->second--;
                                }
                            }
                            print "Closed connection to client " << fd << endl;
                            continue;
                        }
                    }

                    // Handle writable client
                    if (events[i].events & EPOLLOUT) {
                        int result = client->handleWriteEvent();
                        if (result == FINISHED) {
                            // Switch back to reading for keep-alive
                            struct epoll_event event;
                            event.events = EPOLLIN | EPOLLET;
                            event.data.fd = fd;
                            if (epoll_ctl(this->_epollFd, EPOLL_CTL_MOD, fd, &event) < 0) {
                                print "Failed to switch client to read mode: " << strerror(errno) << endl;
                                // Close connection on error
                                epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
                                close(fd);
                                clientToServer.erase(fd);
                                fdToClientIndex.erase(clientIt);
                                delete this->_clients[clientIndex];
                                this->_clients.erase(this->_clients.begin() + clientIndex);
                                for (std::map<int, int>::iterator mapIt = fdToClientIndex.begin();
                                     mapIt != fdToClientIndex.end(); ++mapIt) {
                                    if (mapIt->second > clientIndex) {
                                        mapIt->second--;
                                    }
                                }
                                continue;
                            }
                        } else if (result == DISCONNECTED) {
                            epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
                            close(fd);
                            clientToServer.erase(fd);
                            fdToClientIndex.erase(clientIt);
                            delete this->_clients[clientIndex];
                            this->_clients.erase(this->_clients.begin() + clientIndex);
                            for (std::map<int, int>::iterator mapIt = fdToClientIndex.begin();
                                 mapIt != fdToClientIndex.end(); ++mapIt) {
                                if (mapIt->second > clientIndex) {
                                    mapIt->second--;
                                }
                            }
                            print "Closed connection to client " << fd << endl;
                            continue;
                        }
                    }
                }
            }
        }
    }

    close(this->_epollFd);
}


void	Config::establishConnection() {
	struct addrinfo hint = addrinfo(), *newAddr;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;

	for (size_t i = 0; i < this->_servers.size(); ++i){
		for (std::set<int>::const_iterator port = this->_servers[i].port.begin(); port != this->_servers[i].port.end(); ++port) {
			this->_servers[i].createSockets(hint, &newAddr, to_string(*port), this->_servers[i].host);
			print "Server listening on " << this->_servers[i].host << ":" << *port << endl;
		}
	}
	print "All server sockets created and listening" << endl;
	
	this->listenForEvents();
}