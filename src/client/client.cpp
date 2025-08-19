#include "../../includes/client.hpp"
#include "../../includes/networking.hpp"
#include <fcntl.h>
#include <cstring>
#include <cerrno>
#include <sstream>

in_addr_t host_to_addr(const std::string& host) {
	stringstream ss(host);
	string line;
	u_int32_t addr = 0;
	while (std::getline(ss, line, '.')) {
		addr = (addr << 8) | std::atoi(line.c_str());
	}
	addr = htonl(addr);
	return (addr);
}

// void Client::makeConnection(int port, const std::string& host) {
// 	struct sockaddr_in serverAddr;
// 	serverAddr.sin_family = AF_INET;
// 	serverAddr.sin_port = htons(static_cast<uint16_t>(port));
// 	serverAddr.sin_addr.s_addr = host_to_addr(host);

// 	this->socketFd = socket(AF_INET, SOCK_STREAM, 0);
// 	if (this->socketFd < 0) {
// 		throw std::runtime_error("Failed to create socket");
// 	}

// 	if (connect(this->socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
// 		close(this->socketFd);
// 		throw std::runtime_error("Failed to connect to server");
// 	}
// }

void Client::fileDescriptorDisposal() {
	for (size_t i = 0; i < this->socketFd.size(); i++) {
		close(this->socketFd[i]);
	}
	for (size_t i = 0; i < this->epollFd.size(); i++) {
		close(this->epollFd[i]);
	}
}

// void Client::makeConnection(int port, const std::string& host) {
// 	struct addrinfo hint, *newAddr;
// 	hint.ai_family = AF_INET;
// 	hint.ai_socktype = SOCK_STREAM;
// 	hint.ai_flags = AI_PASSIVE;

// 	if (getaddrinfo(host.c_str(), to_string(port).c_str(), &hint, &newAddr) != 0) {
// 		throw std::runtime_error("Failed to get address info");
// 	}

// 	this->socketFd = socket(newAddr->ai_family, newAddr->ai_socktype, newAddr->ai_protocol);
// 	if (this->socketFd < 0) {
// 		freeaddrinfo(newAddr);
// 		throw std::runtime_error("Failed to create socket");
// 	}

// 	if (connect(this->socketFd, newAddr->ai_addr, newAddr->ai_addrlen) < 0) {
// 		close(this->socketFd);
// 		freeaddrinfo(newAddr);
// 		throw std::runtime_error("Failed to connect to server");
// 	}

// 	freeaddrinfo(newAddr);
// }

//Will need to refactor the code here for multiple ports instead of one

// int	Client::newConnection() {

// }

void	Client::establishConnection() {
	struct addrinfo hint, *newAddr;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;
	size_t index = 0;

	for (std::set<int>::const_iterator i = this->_config->port.begin(); i != this->_config->port.end(); ++i) {
		newAddr = NULL;
		if (getaddrinfo(this->_config->host.c_str(), to_string(*i).c_str(), &hint, &newAddr) < 0){
			this->fileDescriptorDisposal();
			if (newAddr != NULL) {
				freeaddrinfo(newAddr);
			}
			throw std::runtime_error(gai_strerror(errno));
		}

		
		int fileDescriptor = socket(newAddr->ai_family, newAddr->ai_socktype, newAddr->ai_protocol);
		this->socketFd.insert(this->socketFd.end(), fileDescriptor );
		if (this->socketFd[index] < 0) {
			freeaddrinfo(newAddr);
			throw std::runtime_error("Failed to create socket");
		}

		if (bind(this->socketFd[index], newAddr->ai_addr, newAddr->ai_addrlen) < 0) {
			this->fileDescriptorDisposal();
			freeaddrinfo(newAddr);
			switch (errno) {
				case EADDRINUSE:
					throw std::runtime_error("Address already in use");
				case EACCES:
					throw std::runtime_error("Permission denied");
				default:
					throw std::runtime_error("Failed to bind socket");
			}
		}

		if (fcntl(this->socketFd[index], F_SETFL, O_NONBLOCK) < 0) {
			close(this->socketFd[index]);
			freeaddrinfo(newAddr);
			throw std::runtime_error("Failed to set socket to non-blocking mode");
		}

		int optval = 1;
		if (setsockopt(this->socketFd[index], SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
			close(this->socketFd[index]);
			freeaddrinfo(newAddr);
			throw std::runtime_error("Failed to set socket options");
		}

		if (listen(this->socketFd[index], SOMAXCONN) < 0) {
			close(this->socketFd[index]);
			freeaddrinfo(newAddr);
			throw std::runtime_error("Failed to listen on socket");
		}

		fileDescriptor = epoll_create(1);
		this->epollFd.insert(this->epollFd.end(), fileDescriptor );
		if (this->epollFd[index] < 0) {
			this->fileDescriptorDisposal();
			throw std::runtime_error("Failed to create epoll instance");
		}

		struct epoll_event event;
		event.events = EPOLLIN;
		event.data.fd = this->socketFd[index];
		if (epoll_ctl(epollFd[index], EPOLL_CTL_ADD, this->socketFd[index], &event) < 0) {
			this->fileDescriptorDisposal();
			freeaddrinfo(newAddr);
			throw std::runtime_error("Failed to add socket to epoll instance");
		}
		
		freeaddrinfo(newAddr);
		index++;
	}

	print "Server is listening on port(s) ";
	for (std::set<int>::const_iterator it = this->_config->port.begin(); it != this->_config->port.end(); ++it) {
		print *it << " ";
	}
	print "at host " << this->_config->host << std::endl;

	struct epoll_event events[10];
	while (true) {
		for (size_t epollIndex = 0; epollIndex < this->epollFd.size(); epollIndex++) {
			int nfds = epoll_wait(this->epollFd[epollIndex], events, 10, 0);
			if (nfds < 0) {
				this->fileDescriptorDisposal();
				throw std::runtime_error("Epoll wait failed");
			}
			
			for (int eventIndex = 0; eventIndex < nfds; ++eventIndex) {
				bool isServerSocket = false;
				size_t serverSocketIndex = 0;
				for (size_t j = 0; j < this->socketFd.size(); j++) {
					if (events[eventIndex].data.fd == this->socketFd[j]) {
						isServerSocket = true;
						serverSocketIndex = j;
						break;
					}
				}
				
				if (isServerSocket) {
					// Will need to craete a method to Handle new connection that will contain the code below
					int clientFd = accept(this->socketFd[serverSocketIndex], NULL, NULL);
					if (clientFd < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							continue;
						}
						this->fileDescriptorDisposal();
						throw std::runtime_error("Failed to accept client connection");
					}

					if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
						close(clientFd);
						continue;
					}

					struct epoll_event clientEvent;
					clientEvent.events = EPOLLIN | EPOLLET;
					clientEvent.data.fd = clientFd;
					if (epoll_ctl(this->epollFd[epollIndex], EPOLL_CTL_ADD, clientFd, &clientEvent) < 0) {
						close(clientFd);
						continue;
					}
				
					print "Accepted new client connection on socket " << clientFd << endl;
				} else {
					//This will be moved to another method that will be called preestablishedConnenction()
					int clientFd = events[eventIndex].data.fd;
					string fullRequest = "";
					char buffer[1024];
					ssize_t totalBytesRead = 0;

					//Below will be the request and the response
					while (true) {
						ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
						
						if (bytesRead > 0) {
							buffer[bytesRead] = '\0';
							fullRequest += string(buffer);
							totalBytesRead += bytesRead;
						} else if (bytesRead == 0) {
							print "Client " << clientFd << " disconnected" << endl;
							epoll_ctl(this->epollFd[epollIndex], EPOLL_CTL_DEL, clientFd, NULL);
							close(clientFd);
							break;
						} else {
							if (errno == EAGAIN || errno == EWOULDBLOCK) {
								if (totalBytesRead > 0) {
									print "Received " << totalBytesRead << " bytes from client " << clientFd << endl;
									
									Client clientHandler(clientFd, this->_config);
									clientHandler.setRequestBuf(fullRequest);
									
									for (int i = 0; i < 10; ++i) {
										ClientState prevState = clientHandler.state;
										clientHandler.processRequest();
										if (clientHandler.state == prevState) break;
										if (clientHandler.state == READING_DONE || clientHandler.state == READING_BAD_REQUEST) break;
									}
									
									string response;
									if (clientHandler.httpStatusCode == 400 && clientHandler.state == READING_BODY) {
										std::string method = clientHandler.request.getMethod();
										if (method == "POST") {
											response = "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nPOST request done!";
										} else {
											response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
										}
									} else if (clientHandler.httpStatusCode != 0 && clientHandler.httpStatusCode != 200) {
										if (clientHandler.httpStatusCode == 400) {
											response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
										} else if (clientHandler.httpStatusCode == 413) {
											response = "HTTP/1.1 413 Payload Too Large\r\nContent-Length: 18\r\n\r\nPayload Too Large";
										} else if (clientHandler.httpStatusCode == 501) {
											response = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 15\r\n\r\nNot Implemented";
										} else {
											response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server Error";
										}
									} else {
										std::string method = clientHandler.request.getMethod();
										if (method == "GET") {
											string uri = clientHandler.request.getUri();
											string body = "<html><body><h1>Hello from WebServ!</h1><p>GET request to: " + uri + "</p></body></html>";
											stringstream ss;
											ss << body.length();
											response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + ss.str() + "\r\n\r\n" + body;
										} else if (method == "POST") {
											response = "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nPOST request done!";
										} else if (method == "DELETE") {
											response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nDELETE request done!";
										} else if (method == "UNSUPPORTED") {
											response = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 15\r\n\r\nNot Implemented";
										} else {
											response = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
										}
									}
									
									send(clientFd, response.c_str(), response.length(), 0);
									
									epoll_ctl(this->epollFd[epollIndex], EPOLL_CTL_DEL, clientFd, NULL);
									close(clientFd);
								}
								break;
							} else {
								print "Error reading from client " << clientFd << ": " << strerror(errno) << endl;
								epoll_ctl(this->epollFd[epollIndex], EPOLL_CTL_DEL, clientFd, NULL);
								close(clientFd);
								break;
							}
						}
					}
				}
			}
		}
	}
	this->fileDescriptorDisposal();
}

void    initialiseServer(Config& config) {
	for (vector<server>::const_iterator it = config._servers.begin(); it != config._servers.end(); ++it) {
		DEBUG;
		try {
			for (std::set<int>::const_iterator portIt = it->port.begin(); portIt != it->port.end(); ++portIt) {
				Client _client(0, &(*it));
				_client.establishConnection();
				config._clients.insert(config._clients.end(), _client);
				print *portIt << " on host " << it->host << endl;
			}
		} catch (const std::exception& e) {
			throw std::runtime_error("Error establishing connection: " + std::string(e.what()));
		}
	}
}

// Client::Client() : socketFd(-1), contentLen(0), state(READING_REQUEST_LINE),
// 	request(this->_config->maxBodySize), response(this->_config, request), httpStatusCode(0) {
// }

Client& Client::operator=(const Client& other) {
	if (this != &other) {
		this->socketFd = other.socketFd;
		this->requestBuf = other.requestBuf;
		this->responseBuf = other.responseBuf;
		this->contentLen = other.contentLen;
		this->state = other.state;
		// this->request = other.request; // Will need to implement this
		this->response = other.response; // This One too
		this->httpStatusCode = other.httpStatusCode;
	}
	return *this;
}

Client::Client(int fd, const server* serverConfig) :
	_config(serverConfig), socketFd(fd), contentLen(0),
	state(READING_REQUEST_LINE), request(_config->maxBodySize),
	response(serverConfig, request), httpStatusCode(0) {}

void    Client::updateState() {
	switch (this->request.getState()) {
		case PARSINGREQUESTLINE:
			this->state = READING_REQUEST_LINE; break;
		case REQUESTLINE_READY:
			this->state = READING_HEADERS; break;
		case REQUESTHEADERS_READY:
			this->state = (this->request.getMethod() == "POST") ?
							READING_BODY : READING_DONE;
			break;
		case PARSINGCOMPLETE:
			this->state = READING_DONE; break;
		case PARSINGERROR:
			this->state = READING_BAD_REQUEST; break;
		default:
			this->state = READING_DONE; break;
	}
}

void    Client::processRequest() {
	switch (this->state)
	{
	case READING_REQUEST_LINE: 
		this->httpStatusCode = this->request.processRequestLine(this->requestBuf); break;
	case READING_HEADERS:
		this->httpStatusCode = this->request.processHeaders(this->requestBuf); break;
	case READING_BODY:
		this->httpStatusCode = this->request.processBody(this->requestBuf); break;
		break;
	default:
		break;
	}
	updateState();
}

void    Client::generateResponse() {
	if (this->httpStatusCode != 0) {
		cout << this->httpStatusCode << endl;
		return ;
	}
	// response.checkRequirements();
	// response.constructResourcePath();
}