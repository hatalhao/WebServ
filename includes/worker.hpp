#pragma once

#include "Config.hpp"
#include "Webserv.hpp"
#include "client.hpp"
#include "cookies.hpp"
#include "networking.hpp"


class Worker {
	private:
		int									_epollFd;
		std::map<int, Client*>				_clients;
		std::vector<server>					_servers;
		std::map<int, CgiHandler*>			_cgiHandlers;
		std::set<CgiHandler*>				_cgiSet;
		SessionManager						_sessionManager;
		static Worker*						instance;
		std::map< std::pair<std::string,int>, std::set<int> > hostToIndex;
		std::map<int, std::pair<int, int> > socketToServer;
		std::map<int, int> clientToServer;
		std::map<int, int> socketToPort;

		void								initializeEpoll();
		void								closeClient(int fd, std::map<int, Client*>::iterator& clientIt);
		void								handleServerSocket(int fd);
		void								handleCgiEvent(int fd, uint32_t events);
		void								handleClientEvent(int fd, uint32_t events, std::map<int, Client*>::iterator& clientIt);

		Worker(const Worker &other){
			(void) other;
		};
		Worker &operator=(const Worker &other){
			(void) other;
			return *this;
		};
		Worker(const std::string& filename) : _epollFd(-1), _servers(Config(filename).getServers()){};
		~Worker();
	
	public:
		static Worker*						getInstance(const std::string& filename);
		static void							cleanup();
		int									getEpollFd() { return _epollFd; }
		SessionManager&						getSessionManager() { return _sessionManager; }
		std::vector<server>&				getServers() { return _servers; }
		void								setServers(std::vector<server> parsedServers) { _servers = parsedServers; }
		std::map<int, CgiHandler*>&			getCgiHandlers() { return _cgiHandlers; }
		std::set<CgiHandler*>&				getCgiSet() { return _cgiSet; }
		void								establishConnection();
		int									acceptConnection(int serverIndex, int socketIndex);
		void								listenForEvents();
		void								checkCgiTimeouts();
		void								cleanupFinishedCgi();
		void								cleanupIdleConnections();
		std::set<int>&						getCondidateServersIndices(const std::pair<std::string, int>& hostPort) { return hostToIndex[hostPort]; }
};
