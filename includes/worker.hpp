#pragma once

#include "origin.hpp"
#include "Webserv.hpp"
#include "cookies.hpp"
#include "networking.hpp"


class Worker {
	private:
		Worker(const std::string& filename) : _epollFd(-1), _servers(Config(filename)._servers){};

		~Worker(){ delete instance; };
		Worker(const Worker &other){};
		Worker &operator=(const Worker &other){};
		static Worker* instance;

		int									_epollFd;
		std::vector<Client*>				_clients;
		std::vector<server>					_servers;
		std::map<int, CgiHandler*>			_cgiHandlers;
		std::set<CgiHandler*>				_cgiSet;
		SessionManager						_sessionManager;

	public:
		static Worker* getInstance(const std::string& filename) {
			if (!instance)
				instance = new Worker(filename);
			return instance;
		}

		int									getEpollFd() { return _epollFd; }
		SessionManager&						getSessionManager() { return _sessionManager; }
		std::vector<Client*>&				getClients() { return _clients; }
		std::vector<server>&				getServers() { return _servers; }
		void								setServers(std::vector<server> parsedServers) { _servers = parsedServers; }
		std::map<int, CgiHandler*>&			getCgiHandlers() { return _cgiHandlers; }
		std::set<CgiHandler*>&				getCgiSet() { return _cgiSet; }
				void						establishConnection();
		int									acceptConnection(int serverIndex, int socketIndex);
		void								listenForEvents();
		void								checkCgiTimeouts();
		void								cleanupFinishedCgi();
		SessionManager&						getSessionManager() { return _sessionManager; }

};
