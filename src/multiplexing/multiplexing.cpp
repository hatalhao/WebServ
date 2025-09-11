#include "../../includes/Config.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/networking.hpp"
#include "../../includes/client.hpp"

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
	Client* newClient = new Client(clientFd, this, serverIndex);
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
            // new cgi case
            else if (this->_cgiHandlers.find(fd) != this->_cgiHandlers.end()) {
                CgiHandler  *handler = this->_cgiHandlers[fd];
                // For pipes under EPOLLET, treat HUP/ERR on read side as readable to drain & finalize
                if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR))
                    handler->manageReadEvent(this->_epollFd);
                if (events[i].events & EPOLLOUT)
                    handler->manageWriteEvent(this->_epollFd);
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

        // Cleanup finished CGI handlers using the tracked set to avoid UAF
        for (std::set<CgiHandler*>::iterator sit = this->_cgiSet.begin(); sit != this->_cgiSet.end(); ) {
            CgiHandler* h = *sit;
            if (h->getState() == CGI_DONE) {
                // Remove all fds registered for this handler
                std::vector<int> keys;
                for (std::map<int, CgiHandler*>::iterator jt = this->_cgiHandlers.begin(); jt != this->_cgiHandlers.end(); ++jt) {
                    if (jt->second == h) keys.push_back(jt->first);
                }
                for (size_t k = 0; k < keys.size(); ++k) {
                    epoll_ctl(this->_epollFd, EPOLL_CTL_DEL, keys[k], NULL);
                    this->_cgiHandlers.erase(keys[k]);
                }
                // Reap child with waitpid(WNOHANG) could be added here
                delete h;
                this->_cgiSet.erase(sit++);
            } else {
                ++sit;
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