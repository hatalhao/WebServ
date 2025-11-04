#pragma once

#include "Config.hpp"
#include "Webserv.hpp"
#include "client.hpp"
#include "Session.hpp"
#include "networking.hpp"


class Worker {
	private:
		int									_epollFd;
		map<int, Client*>					_clients;
		vector<server>						_servers;
		map<int, CgiHandler*>				_cgiHandlers;
		set<CgiHandler*>					_cgiSet;
		SessionManager						_sessionManager;
		static Worker*						instance;
		map< pair<string,int>, set<int> >	hostToIndex;
		map<int, pair<int, int> >			socketToServer;
		map<int, int>						socketToPort;

		void								initializeEpoll();
		void								closeClient(int fd, map<int, Client*>::iterator& clientIt);
		void								handleServerSocket(int fd);
		void								handleCgiEvent(int fd, uint32_t events);
		void								handleClientEvent(int fd, uint32_t events, map<int, Client*>::iterator& clientIt);

		Worker(const std::string& filename) : _epollFd(-1), _servers(Config(filename).getServers()){};
		~Worker();
	
	public:
		static Worker*						getInstance(const string& filename);
		static void							cleanup();
		int									getEpollFd() { return _epollFd; }
		SessionManager&						getSessionManager() { return _sessionManager; }
		vector<server>&						getServers() { return _servers; }
		void								setServers(vector<server> parsedServers) { _servers = parsedServers; }
		map<int, CgiHandler*>&				getCgiHandlers() { return _cgiHandlers; }
		set<CgiHandler*>&					getCgiSet() { return _cgiSet; }
		void								establishConnection();
		int									acceptConnection(int serverIndex, int socketIndex);
		void								listenForEvents();
		void								checkCgiTimeouts();
		void								cleanupFinishedCgi();
		void								cleanupIdleConnections();
		set<int>&							getCondidateServersIndices(const pair<string, int>& hostPort) { return hostToIndex[hostPort]; }
};
