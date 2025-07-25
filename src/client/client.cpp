#include "../../includes/client.hpp"
#include "../../includes/networking.hpp"
#include <fcntl.h>

in_addr_t host_to_addr(const std::string& host) {
	stringstream ss(host);
	string line;
	u_int32_t addr = 0;
	while (std::getline(ss, line, '.')) {
		addr = (addr << 8) | std::atoi(line.c_str());
	}
	addr = ntohl(addr);
	return (addr);
}

void Client::makeConnection(int port, const std::string& host) {
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(static_cast<uint16_t>(port));
	serverAddr.sin_addr.s_addr = host_to_addr(host);

	this->socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->socketFd < 0) {
		throw std::runtime_error("Failed to create socket");
	}

	if (connect(this->socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		close(this->socketFd);
		throw std::runtime_error("Failed to connect to server");
	}
}

//socket(), bind(), listen(), send(), epoll(), accept(), connect() functions
void	Client::establishConnection(int port, const std::string& host) {
	struct sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(static_cast<uint16_t>(port));
	serverAddr.sin_addr.s_addr = host_to_addr(host);

	this->socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->socketFd < 0) {
		throw std::runtime_error("Failed to create socket");
	}

	if (bind(this->socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		close(this->socketFd);
		switch (errno) {
			case EADDRINUSE:
				throw std::runtime_error("Address already in use");
			case EACCES:
				throw std::runtime_error("Permission denied");
			default:
				throw std::runtime_error("Failed to bind socket");
		}
	}

	if (fcntl(this->socketFd, F_SETFL, O_NONBLOCK) < 0) {
		close(this->socketFd);
		throw std::runtime_error("Failed to set socket to non-blocking mode");
	}

	if (setsockopt(this->socketFd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) < 0) {
		close(this->socketFd);
		throw std::runtime_error("Failed to set socket options");
	}

	if (listen(this->socketFd, SOMAXCONN) < 0) {
		close(this->socketFd);
		throw std::runtime_error("Failed to listen on socket");
	}

	int epollFd = epoll_create(1);
	if (epollFd < 0) {
		close(this->socketFd);
		throw std::runtime_error("Failed to create epoll instance");
	}

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = this->socketFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, this->socketFd, &event) < 0) {
		close(epollFd);
		close(this->socketFd);
		throw std::runtime_error("Failed to add socket to epoll instance");
	}
	print "Server is listening on port " << port << " at host " << host << endl;
	struct epoll_event events[10];
	while (true) {
		int nfds = epoll_wait(epollFd, events, 10, -1);
		if (nfds < 0) {
			close(epollFd);
			close(this->socketFd);
			throw std::runtime_error("Epoll wait failed");
		}
		for (int i = 0; i < nfds; ++i) {
			if (events[i].data.fd == this->socketFd) {
				int clientFd = accept(this->socketFd, NULL, NULL);
				if (clientFd < 0) {
					close(epollFd);
					close(this->socketFd);
					throw std::runtime_error("Failed to accept client connection");
				}
				print "Accepted new client connection on socket " << clientFd << endl;
			}
		}
	}
	// Cleanup
	close(epollFd);
	close(this->socketFd);
}

void    initialiseServer(Config& config) {
	for (vector<server>::const_iterator it = config._servers.begin(); it != config._servers.end(); ++it) {
		try {
			for (std::set<int>::const_iterator portIt = it->port.begin(); portIt != it->port.end(); ++portIt) {
				Client _client;
				_client.establishConnection(*portIt, it->host);
				_client.makeConnection(*portIt, it->host);
				config._clients.insert(config._clients.end(), _client);
				print *portIt << " on host " << it->host << endl;
			}
		} catch (const std::exception& e) {
			throw std::runtime_error("Error establishing connection: " + std::string(e.what()));
		}
	}
}

Client::Client() : socketFd(-1), contentLen(0), state(READING_REQUEST_LINE),
	request(NULL), httpStatusCode(0) {
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
	response.checkRequirements();
	response.constructResourcePath();
}