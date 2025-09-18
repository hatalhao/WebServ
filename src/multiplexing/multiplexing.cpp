#include "../../includes/worker.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/networking.hpp"
#include "../../includes/client.hpp"

int	Worker::acceptConnection(int serverIndex, int socketIndex) {
	if (serverIndex >= static_cast<int>(this->_servers.size()) || 
		socketIndex >= static_cast<int>(this->_servers[serverIndex].socketFd.size())) {
		return -1;
	}
	
	int clientFd = accept(this->_servers[serverIndex].socketFd[socketIndex], NULL, NULL);
	if (clientFd < 0) {
		return -1;
	}

    // Prevent client sockets from being inherited by exec'ed CGI children
    int cflags = fcntl(clientFd, F_GETFD);
    if (cflags >= 0) {
        fcntl(clientFd, F_SETFD, cflags | FD_CLOEXEC);
    }

    int nbFlags = fcntl(clientFd, F_GETFL, 0);
    if (nbFlags < 0 || fcntl(clientFd, F_SETFL, nbFlags | O_NONBLOCK) < 0) {
		close(clientFd);
		return -1;
	}

	// Create client on heap to avoid stack-use-after-return
	Client* newClient = new Client(clientFd, this,
        std::make_pair(this->_servers[serverIndex].host, 
            this->socketToPort[this->_servers[serverIndex].socketFd[socketIndex]]) );
	this->_clients.push_back(newClient);
	int clientIndex = this->_clients.size() - 1;

    struct epoll_event ev;
    // Level-triggered: only watch for readability on new client sockets
    ev.events = EPOLLIN;
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


void Worker::listenForEvents() {
    print "Starting epoll-based event loop" << endl;

    if (this->_servers.empty()) {
        throw std::runtime_error("No servers configured");
    }

    this->_epollFd = epoll_create1(0);
    if (this->_epollFd < 0) {
        throw std::runtime_error("Failed to create epoll instance");
    }
    // Use the shared CGI handlers map from Config to track pipe FDs
    
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

            this->socketToServer[serverSocket] = std::make_pair(serverIndex, socketFdIndex);
        }
    }

    struct epoll_event events[MAXEVENTS];

    while (true) {
        // Compute epoll_wait timeout so we can trigger CGI timeouts even when idle
        int waitTimeoutMs = -1; // -1 => wait indefinitely when no active CGI
        if (!this->_cgiSet.empty()) {
            time_t now = time(NULL);
            bool hasActive = false;
            for (std::set<CgiHandler*>::iterator sit = this->_cgiSet.begin(); sit != this->_cgiSet.end(); ++sit) {
                CgiHandler* h = *sit;
                if (h->getState() == CGI_DONE)
                    continue;
                hasActive = true;
                double elapsed = difftime(now, h->getStartTime());
                double remaining = CGI_TIMEOUT_SECONDS - elapsed;
                int remMs = (remaining <= 0) ? 0 : static_cast<int>(remaining * 1000);
                if (waitTimeoutMs == -1 || remMs < waitTimeoutMs)
                    waitTimeoutMs = remMs;
            }
            if (!hasActive)
                waitTimeoutMs = -1;
        }

        int eventCount = epoll_wait(this->_epollFd, events, MAXEVENTS, waitTimeoutMs);
        if (eventCount < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < eventCount; ++i) {
            int fd = events[i].data.fd;

            // New incoming connection?
            if (this->socketToServer.find(fd) != this->socketToServer.end()) {
                std::pair<int, int> serverInfo = this->socketToServer[fd];
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
                    // Level-triggered: start watching for read events
                    clientEvent.events = EPOLLIN; // start with read
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

                    this->clientToServer[clientFd] = serverIndex;
                    this->fdToClientIndex[clientFd] = this->_clients.size() - 1;
                }
            }
            // new cgi case
            else if (this->_cgiHandlers.find(fd) != this->_cgiHandlers.end()) {
                CgiHandler  *handler = this->_cgiHandlers[fd];
                // For pipes, treat HUP/ERR on read side similar to readable to drain & finalize
                if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR))
                    handler->manageReadEvent(this->_epollFd);
                if (events[i].events & EPOLLOUT)
                    handler->manageWriteEvent(this->_epollFd);
            }
            // Existing client activity
            else {
                std::map<int, int>::iterator clientIt = this->fdToClientIndex.find(fd);
                if (clientIt != this->fdToClientIndex.end()) {
                    int clientIndex = clientIt->second;
                    Client* client = this->_clients[clientIndex];

                    // Error or hangup → close client
                    if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                        epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        this->clientToServer.erase(fd);
                        this->fdToClientIndex.erase(clientIt);

                        delete this->_clients[clientIndex];
                        this->_clients.erase(this->_clients.begin() + clientIndex);

                        for (std::map<int, int>::iterator mapIt = this->fdToClientIndex.begin();
                             mapIt != this->fdToClientIndex.end(); ++mapIt) {
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
                            this->clientToServer.erase(fd);
                            this->fdToClientIndex.erase(clientIt);

                            delete this->_clients[clientIndex];
                            this->_clients.erase(this->_clients.begin() + clientIndex);

                            for (std::map<int, int>::iterator mapIt = this->fdToClientIndex.begin();
                                 mapIt != this->fdToClientIndex.end(); ++mapIt) {
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
                            // Switch back to level-triggered read
                            event.events = EPOLLIN;
                            event.data.fd = fd;
                            if (epoll_ctl(this->_epollFd, EPOLL_CTL_MOD, fd, &event) < 0) {
                                print "Failed to switch client to read mode: " << strerror(errno) << endl;
                                // Close connection on error
                                epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
                                close(fd);
                                this->clientToServer.erase(fd);
                                this->fdToClientIndex.erase(clientIt);
                                delete this->_clients[clientIndex];
                                this->_clients.erase(this->_clients.begin() + clientIndex);
                                for (std::map<int, int>::iterator mapIt = this->fdToClientIndex.begin();
                                     mapIt != this->fdToClientIndex.end(); ++mapIt) {
                                    if (mapIt->second > clientIndex) {
                                        mapIt->second--;
                                    }
                                }
                                continue;
                            }
                        } else if (result == DISCONNECTED) {
                            epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, fd, NULL);
                            close(fd);
                            this->clientToServer.erase(fd);
                            this->fdToClientIndex.erase(clientIt);
                            delete this->_clients[clientIndex];
                            this->_clients.erase(this->_clients.begin() + clientIndex);
                            for (std::map<int, int>::iterator mapIt = this->fdToClientIndex.begin();
                                 mapIt != this->fdToClientIndex.end(); ++mapIt) {
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

    this->checkCgiTimeouts();
    this->cleanupFinishedCgi();

    }

    close(this->_epollFd);
}

void	Worker::establishConnection() {
	struct addrinfo hint = addrinfo(), *newAddr;
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_flags = AI_PASSIVE;

    // Track which ports we've already created sockets for and to implement Virtual Hosting
	for (size_t i = 0; i < this->_servers.size(); ++i){
        for (std::set<int>::const_iterator port = this->_servers[i].port.begin(); port != this->_servers[i].port.end(); ++port) {
            if (this->hostToIndex[make_pair(this->_servers[i].host, *port)].empty()) {
                this->_servers[i].createSockets(hint, &newAddr, to_string(*port), this->_servers[i].host);
                this->socketToPort[this->_servers[i].socketFd.back()] = *port;
            }
            this->hostToIndex[make_pair(this->_servers[i].host, *port)].insert(i);
            print "Server listening on " << this->_servers[i].host << ":" << *port << endl;
		}
	}

    for (map< pair<string,int>, set<int> >::iterator it = this->hostToIndex.begin(); it != this->hostToIndex.end(); ++it) {
        print "Host " << it->first.first << " Port " << it->first.second << " serves servers: ";
        for (set<int>::iterator sit = it->second.begin(); sit != it->second.end(); ++sit) {
            print *sit << " ";
        }
        print endl;
    }
	print "All server sockets created and listening" << endl;
	
	this->listenForEvents();
}

void Worker::checkCgiTimeouts() {
    time_t currentTime = time(NULL);

    // Iterate over unique handler set to avoid processing the same handler twice
    for (std::set<CgiHandler*>::iterator sit = _cgiSet.begin(); sit != _cgiSet.end(); ++sit) {
        CgiHandler* handler = *sit;
        if (!handler)
            continue;
        if (handler->getState() == CGI_DONE)
            continue;

        if (difftime(currentTime, handler->getStartTime()) > CGI_TIMEOUT_SECONDS) {
            // Kill timed-out CGI child process
            pid_t pid = handler->getPid();
            if (pid > 0)
                kill(pid, SIGKILL);

            // Send 504 Gateway Timeout to the client
            handler->getClient().onCgiError(504);

            // Mark handler done
            handler->setState(CGI_DONE);

            // Remove any epoll registrations for this handler's fds and close them
            int rfd = handler->getReadFd();
            int wfd = handler->getWriteFd();
            if (rfd >= 0) {
                epoll_ctl(_epollFd, EPOLL_CTL_DEL, rfd, NULL);
                close(rfd);
            }
            if (wfd >= 0) {
                epoll_ctl(_epollFd, EPOLL_CTL_DEL, wfd, NULL);
                close(wfd);
            }
        }
    }
}

void Worker::cleanupFinishedCgi() {
    // Clean up finished CGI handlers exactly once, removing all their fd registrations
    for (std::set<CgiHandler*>::iterator sit = _cgiSet.begin(); sit != _cgiSet.end(); ) {
        CgiHandler* handler = *sit;
        if (!handler || handler->getState() != CGI_DONE) {
            ++sit;
            continue;
        }

        int status = 0;
        pid_t result = waitpid(handler->getPid(), &status, WNOHANG);
        if (result == 0) { // child not reaped yet
            ++sit;
            continue;
        }

        // Remove all epoll registrations for this handler and erase from map
        std::vector<int> keys;
        for (std::map<int, CgiHandler*>::iterator it = _cgiHandlers.begin(); it != _cgiHandlers.end(); ++it) {
            if (it->second == handler) keys.push_back(it->first);
        }
        for (size_t i = 0; i < keys.size(); ++i) {
            epoll_ctl(_epollFd, EPOLL_CTL_DEL, keys[i], NULL);
            _cgiHandlers.erase(keys[i]);
        }

        // Finally, delete handler and erase from the set
        delete handler;
        _cgiSet.erase(sit++);
    }
}