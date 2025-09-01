#include "../../includes/Webserv.hpp"
#include "../../includes/networking.hpp"

void server::fileDescriptorDisposal() {
	for (size_t i = 0; i < this->socketFd.size(); i++) {
		close(this->socketFd[i]);
	}
}

void server::createSockets(const struct addrinfo& hint, struct addrinfo **newAddr, const std::string& port, const std::string& host) {
	*newAddr = 0;

	int returnValue = getaddrinfo(host.c_str(), port.c_str(), &hint, newAddr);
	if (returnValue < 0){
		this->fileDescriptorDisposal();
		if (newAddr != NULL) { freeaddrinfo(*newAddr); }
		throw std::runtime_error(gai_strerror(returnValue));
	}

	int fileDescriptor = socket((*newAddr)->ai_family, (*newAddr)->ai_socktype, (*newAddr)->ai_protocol);
	this->socketFd.insert(this->socketFd.end(), fileDescriptor );
	if (this->socketFd.back() < 0) {
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to create socket");
	}
	
	int optval = 1;
	if (setsockopt(this->socketFd.back(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) {
		close(this->socketFd.back());
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to set socket options");
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

	if (listen(this->socketFd.back(), SOMAXCONN) < 0) {
		close(this->socketFd.back());
		freeaddrinfo(*newAddr);
		throw std::runtime_error("Failed to listen on socket");
	}

	freeaddrinfo(*newAddr);
}
