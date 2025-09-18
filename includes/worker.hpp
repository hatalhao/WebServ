#pragma once

#include "Config.hpp"
#include "Webserv.hpp"
#include "client.hpp"
#include "cookies.hpp"
#include "networking.hpp"


class Worker {
	private:
		int									_epollFd;
		std::vector<Client*>				_clients;
		std::vector<server>					_servers;
		std::map<int, CgiHandler*>			_cgiHandlers;
		std::set<CgiHandler*>				_cgiSet;
		SessionManager						_sessionManager;
		static Worker*						instance;
		std::map< std::pair<std::string,int>, std::set<int> > hostToIndex;
		std::map<int, std::pair<int, int> > socketToServer;
		std::map<int, int> clientToServer;
		std::map<int, int> fdToClientIndex;
		std::map<int, int> socketToPort; // Map socket fd to port number for Virtual Hosting


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

	set<int>&							getCondidateServersIndices(const pair<string, int>& hostPort) { return hostToIndex[hostPort]; }

};
