#include "../../includes/worker.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/networking.hpp"
#include "../../includes/client.hpp"

static inline bool setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

static inline void setCloseOnExec(int fd) {
	int flags = fcntl(fd, F_GETFD);
	if (flags >= 0)
		fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
}

inline int Worker::acceptConnection(int serverIndex, int socketIndex) {
	if (serverIndex >= static_cast<int>(_servers.size()) ||
		socketIndex >= static_cast<int>(_servers[serverIndex].socketFd.size()))
		return -1;

	int clientFd = accept(_servers[serverIndex].socketFd[socketIndex], NULL, NULL);
	if (clientFd < 0)
		return -1;

	setCloseOnExec(clientFd);
	if (!setNonBlocking(clientFd)) {
		close(clientFd);
		return -1;
	}

	Client* newClient = new Client(clientFd, this,
		std::make_pair(_servers[serverIndex].host,
		socketToPort[_servers[serverIndex].socketFd[socketIndex]]));

	_clients[clientFd] = newClient;

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = clientFd;
	
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
		perror("epoll_ctl: clientFd");
		close(clientFd);
		delete newClient;
		_clients.erase(clientFd);
		return -1;
	}

	return clientFd;
}


static inline int computeNextTimeout(const std::set<CgiHandler*>& cgiSet, 
                              const std::map<int, Client*>& clients, 
                              time_t now) {
	int timeoutMs = -1;

	for (std::set<CgiHandler*>::const_iterator it = cgiSet.begin(); it != cgiSet.end(); ++it) {
		if ((*it)->getState() == CGI_DONE)
			continue;
		double remaining = CGI_TIMEOUT_SECONDS - difftime(now, (*it)->getStartTime());
		int remMs = (remaining <= 0) ? 0 : static_cast<int>(remaining * 1000);
		if (timeoutMs == -1 || remMs < timeoutMs)
			timeoutMs = remMs;
	}

	for (std::map<int, Client*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
		double remaining = KEEP_ALIVE_TIMEOUT - difftime(now, it->second->getLastActivity());
		int remMs = (remaining <= 0) ? 0 : static_cast<int>(remaining * 1000);
		if (timeoutMs == -1 || remMs < timeoutMs)
			timeoutMs = remMs;
	}

	return timeoutMs;
}

inline void Worker::initializeEpoll() {
	if (_servers.empty())
		throw std::runtime_error("No servers configured");

	_epollFd = epoll_create1(0);
	if (_epollFd < 0)
		throw std::runtime_error("Failed to create epoll instance");

	for (size_t i = 0; i < _servers.size(); ++i) {
		for (size_t j = 0; j < _servers[i].socketFd.size(); ++j) {
			int serverSocket = _servers[i].socketFd[j];

			if (!setNonBlocking(serverSocket)) {
				close(_epollFd);
				throw std::runtime_error("Failed to set server socket non-blocking");
			}

			struct epoll_event event;
			event.events = EPOLLIN;
			event.data.fd = serverSocket;

			if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, serverSocket, &event) < 0) {
				close(_epollFd);
				throw std::runtime_error("Failed to add server socket to epoll");
			}

			socketToServer[serverSocket] = std::make_pair(i, j);
		}
	}
}

inline void Worker::closeClient(int fd, std::map<int, Client*>::iterator& clientIt) {
	Client* client = clientIt->second;
	
	// Mark associated CGI handler as done to prevent use-after-free
	if (client->getCgiHandler()) {
		client->getCgiHandler()->setState(CGI_DONE);
	}
	
	epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	delete client;
	_clients.erase(clientIt);
}

inline void Worker::handleServerSocket(int fd) {
	std::pair<int, int> serverInfo = socketToServer[fd];
	acceptConnection(serverInfo.first, serverInfo.second);
}

inline void Worker::handleCgiEvent(int fd, uint32_t events) {
	CgiHandler* handler = _cgiHandlers[fd];
	if (events & (EPOLLIN | EPOLLHUP | EPOLLERR))
		handler->manageReadEvent(_epollFd);
	if (events & EPOLLOUT)
		handler->manageWriteEvent(_epollFd);
}

inline void Worker::handleClientEvent(int fd, uint32_t events, std::map<int, Client*>::iterator& clientIt) {
	Client* client = clientIt->second;

	if (events & (EPOLLERR | EPOLLHUP)) {
		closeClient(fd, clientIt);
		return;
	}

	if (events & EPOLLIN) {
		int result = client->handleReadEvent(_epollFd);
		if (result == DISCONNECTED || result == FINISHED)
			return closeClient(fd, clientIt);
	}

	if (events & EPOLLOUT) {
		int result = client->handleWriteEvent();
		if (result == FINISHED) {
			struct epoll_event event;
			event.events = EPOLLIN;
			event.data.fd = fd;
			if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &event) < 0)
				closeClient(fd, clientIt);
		}
		else if (result == DISCONNECTED) 
			closeClient(fd, clientIt);
	}
}

inline void Worker::listenForEvents() {
	initializeEpoll();

	struct epoll_event events[MAXEVENTS];

	while (true) {
		int timeoutMs = computeNextTimeout(_cgiSet, _clients, time(NULL));
		int eventCount = epoll_wait(_epollFd, events, MAXEVENTS, timeoutMs);

		if (eventCount < 0) {
			if (errno == EINTR) continue;
			break;
		}

		for (int i = 0; i < eventCount; ++i) {
			int fd = events[i].data.fd;

			if (socketToServer.find(fd) != socketToServer.end())
				handleServerSocket(fd);
			else if (_cgiHandlers.find(fd) != _cgiHandlers.end())
				handleCgiEvent(fd, events[i].events);
			else {
				std::map<int, Client*>::iterator clientIt = _clients.find(fd);
				if (clientIt != _clients.end())
					handleClientEvent(fd, events[i].events, clientIt);
			}
		}

		checkCgiTimeouts();
		cleanupFinishedCgi();
		cleanupIdleConnections();
	}

	close(_epollFd);
}

void    Worker::establishConnection() {
	struct addrinfo hint = addrinfo(), *newAddr;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;

	for (size_t i = 0; i < _servers.size(); ++i) {
		for (std::set<int>::const_iterator port = _servers[i].port.begin();
			 port != _servers[i].port.end(); ++port) {
			std::pair<std::string, int> hostPort = std::make_pair(_servers[i].host, *port);

			if (hostToIndex[hostPort].empty()) {
				_servers[i].createSockets(hint, &newAddr, to_string(*port), _servers[i].host);
				socketToPort[_servers[i].socketFd.back()] = *port;
				print "Server listening on " << _servers[i].host << ":" << *port << endl;
			}
			hostToIndex[hostPort].insert(i);
		}
	}

	listenForEvents();
}

static void terminateCgiHandler(CgiHandler* handler, int epollFd) {
	pid_t pid = handler->getPid();
	if (pid > 0)
		kill(pid, SIGKILL);

	handler->getClient().onCgiError(504);
	handler->setState(CGI_DONE);

	int rfd = handler->getReadFd();
	int wfd = handler->getWriteFd();
	if (rfd >= 0) {
		epoll_ctl(epollFd, EPOLL_CTL_DEL, rfd, NULL);
		close(rfd);
	}
	if (wfd >= 0) {
		epoll_ctl(epollFd, EPOLL_CTL_DEL, wfd, NULL);
		close(wfd);
	}
}

inline void Worker::checkCgiTimeouts() {
	time_t currentTime = time(NULL);

	for (std::set<CgiHandler*>::iterator it = _cgiSet.begin(); it != _cgiSet.end(); ++it) {
		CgiHandler* handler = *it;
		if (!handler || handler->getState() == CGI_DONE)
			continue;

		if (difftime(currentTime, handler->getStartTime()) > CGI_TIMEOUT_SECONDS)
			terminateCgiHandler(handler, _epollFd);
	}
}

inline void Worker::cleanupFinishedCgi() {
	for (std::set<CgiHandler*>::iterator it = _cgiSet.begin(); it != _cgiSet.end(); ) {
		CgiHandler* handler = *it;
		if (!handler || handler->getState() != CGI_DONE) {
			++it;
			continue;
		}

		int status = 0;
		if (waitpid(handler->getPid(), &status, WNOHANG) == 0) {
			++it;
			continue;
		}

		std::vector<int> fdsToRemove;
		for (std::map<int, CgiHandler*>::iterator mapIt = _cgiHandlers.begin(); 
			 mapIt != _cgiHandlers.end(); ++mapIt) {
			if (mapIt->second == handler)
				fdsToRemove.push_back(mapIt->first);
		}

		for (size_t i = 0; i < fdsToRemove.size(); ++i) {
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, fdsToRemove[i], NULL);
			_cgiHandlers.erase(fdsToRemove[i]);
		}

		delete handler;
		_cgiSet.erase(it++);
	}
}

inline void Worker::cleanupIdleConnections() {
	time_t now = time(NULL);
	std::vector<int> fdsToClose;

	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		Client* client = it->second;
		ClientState clientState = client->getState();
		
		if (clientState == WAITING_FOR_CGI || clientState == SENDING_RESPONSE)
			continue;
		
		double elapsed = difftime(now, client->getLastActivity());
		if (elapsed > KEEP_ALIVE_TIMEOUT) {
			if (clientState == READING_REQUEST_LINE && client->getRequestBuf().empty() && client->shouldKeepAlive()) {
				fdsToClose.push_back(it->first);
			} else if (clientState == READING_REQUEST_LINE || clientState == READING_HEADERS || 
			           clientState == READING_BODY || clientState == PROCESSING_UPLOAD) {
				client->setKeepAlive(false);
				client->onCgiError(408);
			} else
				fdsToClose.push_back(it->first);
		}
	}

	for (size_t i = 0; i < fdsToClose.size(); ++i) {
		std::map<int, Client*>::iterator it = _clients.find(fdsToClose[i]);
		if (it != _clients.end()) {
			epoll_ctl(_epollFd, EPOLL_CTL_DEL, it->first, NULL);
			close(it->first);
			delete it->second;
			_clients.erase(it);
		}
	}
}