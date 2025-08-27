#include "../../includes/client.hpp"
#include "../../includes/networking.hpp"
#include "../../includes/methods.hpp"

void Client::fileDescriptorDisposal() {
	for (size_t i = 0; i < this->socketFd.size(); i++) {
		close(this->socketFd[i]);
	}
	for (size_t i = 0; i < this->epollFd.size(); i++) {
		close(this->epollFd[i]);
	}
}

void	Client::CreateSockets(const struct addrinfo& hint, struct addrinfo **newAddr, const std::string& port, const std::string& host) {
	*newAddr = 0;

	if (getaddrinfo(host.c_str(), port.c_str(), &hint, newAddr) < 0){
		this->fileDescriptorDisposal();
		if (newAddr != NULL) { freeaddrinfo(*newAddr); }
		throw std::runtime_error(gai_strerror(errno));
	}

	int fileDescriptor = socket((*newAddr)->ai_family, (*newAddr)->ai_socktype, (*newAddr)->ai_protocol);
	this->socketFd.insert(this->socketFd.end(), fileDescriptor );
	if (this->socketFd.back() < 0) {
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to create socket");
	}

	if (bind(this->socketFd.back(), (*newAddr)->ai_addr, (*newAddr)->ai_addrlen) < 0) {
		this->fileDescriptorDisposal();
		freeaddrinfo(*newAddr);
		switch (errno) {
			case EADDRINUSE:
				throw std::runtime_error("Address already in use");
			case EACCES:
				throw std::runtime_error("Permission denied");
			default:
				throw std::runtime_error("Failed to bind socket");
		}
	}

	if (fcntl(this->socketFd.back(), F_SETFL, O_NONBLOCK) < 0) {
		close(this->socketFd.back());
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to set socket to non-blocking mode");
	}

	int optval = 1;
	if (setsockopt(this->socketFd.back(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
		close(this->socketFd.back());
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to set socket options");
	}

	if (listen(this->socketFd.back(), SOMAXCONN) < 0) {
		close(this->socketFd.back());
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to listen on socket");
	}

	fileDescriptor = epoll_create(1);
	this->epollFd.insert(this->epollFd.end(), fileDescriptor );
	if (this->epollFd.back() < 0) {
		this->fileDescriptorDisposal();
		throw std::runtime_error("Failed to create epoll instance");
	}

	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = this->socketFd.back();
	if (epoll_ctl(epollFd.back(), EPOLL_CTL_ADD, this->socketFd.back(), &event) < 0) {
		this->fileDescriptorDisposal();
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to add socket to epoll instance");
	}

	freeaddrinfo(*newAddr);
}

void	Client::acceptConnection(int serverSocketIndex, int epollIndex) {
	int clientFd = accept(this->socketFd[serverSocketIndex], NULL, NULL);
	if (clientFd < 0) {
		return ;
	}

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
		close(clientFd);
		return ;
	}

	struct epoll_event clientEvent;
	clientEvent.events = EPOLLIN | EPOLLET;
	clientEvent.data.fd = clientFd;
	if (epoll_ctl(this->epollFd[epollIndex], EPOLL_CTL_ADD, clientFd, &clientEvent) < 0) {
		close(clientFd);
		return ;
	}

	print "Accepted new client connection on socket " << clientFd << endl;
}

int		Client::handleClientData(int clientFd, int epollIndex) {
	static std::map<int, std::string> clientRequests;
	static std::map<int, ssize_t> clientBytesRead;
	
	char buffer[1024];
	ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
	
	if (bytesRead > 0) {
		buffer[bytesRead] = '\0';
		clientRequests[clientFd] += string(buffer);
		clientBytesRead[clientFd] += bytesRead;
		return (CONTINUE);
	} else if (bytesRead == 0) {
		print "Client " << clientFd << " disconnected" << endl;
		epoll_ctl(this->epollFd[epollIndex], EPOLL_CTL_DEL, clientFd, NULL);
		close(clientFd);
		clientRequests.erase(clientFd);
		clientBytesRead.erase(clientFd);
		return (DISCONNECTED);
	} else {
		if (clientBytesRead[clientFd] > 0) {
			print "Received " << clientBytesRead[clientFd] << " bytes from client " << clientFd << endl;
			print "The Full Request is: " << clientRequests[clientFd] << endl;
			
			Client clientHandler(clientFd, this->_config);
			clientHandler.setRequestBuf(clientRequests[clientFd]);
			
			for (int i = 0; i < MAXEVENTS; ++i) {
				ClientState prevState = clientHandler.state;
				clientHandler.processRequest();
				if (clientHandler.state == prevState) break;
				if (clientHandler.state == READING_DONE || clientHandler.state == READING_BAD_REQUEST) break;
			}
			
			clientHandler.generateResponse();

			// this would be changed to be dependant on the state too.
			size_t totalSent = 0;
			size_t toSend = clientHandler.responseBuf.length();
			const char* buf = clientHandler.responseBuf.c_str();
			
			while (totalSent < toSend) {
				ssize_t sent = send(clientFd, buf + totalSent, toSend - totalSent, 0);
				if (sent <= 0) {
					// Handle error or client disconnect
					break;
				}
				totalSent += sent;
			}
			
			epoll_ctl(this->epollFd[epollIndex], EPOLL_CTL_DEL, clientFd, NULL);
			close(clientFd);
			
			clientRequests.erase(clientFd);
			clientBytesRead.erase(clientFd);
		}
		return (FINISHED);
	}
	return (CONTINUE);
}

void	Client::listenForEvents() {
	struct epoll_event events[MAXEVENTS];
	while (true) {
		for (size_t epollIndex = 0; epollIndex < this->epollFd.size(); epollIndex++) {
			int nfds = epoll_wait(this->epollFd[epollIndex], events, MAXEVENTS, 0);
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
					this->acceptConnection(serverSocketIndex, epollIndex);
				} else {
					int clientFd = events[eventIndex].data.fd;
					while (true) {
						int state = this->handleClientData(clientFd, epollIndex);
						
						if (state == DISCONNECTED || state == FINISHED) { break; }
					}
				}
			}
		}
	}
}

void	Client::establishConnection(const Config& totalConfig) {
	struct addrinfo hint, *newAddr;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;

	for (size_t i = 0; i < totalConfig._servers.size(); ++i){
		for (std::set<int>::const_iterator j = totalConfig._servers[i].port.begin(); j != totalConfig._servers[i].port.end(); ++j) {
			this->CreateSockets(hint, &newAddr, to_string(*j), totalConfig._servers[i].host);
		}
	}
	print "All server sockets created and listening" << endl;
	this->listenForEvents();
	this->fileDescriptorDisposal();
}


void    initialiseServer(Config& config) {
	try {
		Client masterClient(0, &config._servers[0]);
		masterClient.establishConnection(config);
		config._clients.insert(config._clients.end(), masterClient);
		
		print "All servers initialized successfully" << endl;
	} catch (const std::exception& e) {
		throw std::runtime_error("Error establishing connection: " + std::string(e.what()));
	}
}

Client& Client::operator=(const Client& other) {
	if (this != &other) {
		_config = other._config;
		socketFd = other.socketFd;
		epollFd = other.epollFd;
		requestBuf = other.requestBuf;
		responseBuf = other.responseBuf;
		contentLen = other.contentLen;
		state = other.state;
		request = other.request;
		response = other.response;
		httpStatusCode = other.httpStatusCode;
	}
	return *this;
}

Client::Client(int fd, server* serverConfig) :
	_config(serverConfig), socketFd(fd), contentLen(0),
	state(READING_REQUEST_LINE), request(_config->maxBodySize),
	response(serverConfig, request, httpStatusCode), httpStatusCode(0) {}

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
	if (this->httpStatusCode != 0)
		{ this->response.serializeResponse(); return ; }

	this->httpStatusCode = response.prepareResponse();

	if (this->request.getMethod() == "GET") {
		this->httpStatusCode = response.Get();
	}
	else if (this->request.getMethod() == "POST") {
		this->httpStatusCode = this->response.Post();
	}
	else if (this->request.getMethod() == "DELETE") {
		
	}
	this->responseBuf = this->response.serializeResponse();
}