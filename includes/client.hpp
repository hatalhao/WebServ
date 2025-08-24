# pragma once

#include "request.hpp"
#include "Config.hpp"
#include "response.hpp"
#include "manipulators.hpp"

enum ClientState {
	READING_REQUEST_LINE,
	READING_HEADERS,
	READING_BODY,
	READING_DONE,
	READING_BAD_REQUEST,
	GENERATING_RESPONSE,
	SENDING_RESPONSE
};


#define MAXEVENTS 1024
#define FINISHED 0
#define DISCONNECTED -1
#define CONTINUE 1

class Client {
	private:
		const server	*_config;
		vector<int>             socketFd;
		vector<int>             epollFd;
		string			requestBuf;
		string			responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			request;
		Response		response;
		unsigned short	httpStatusCode;

	public:
		// Client();
		// Client(Client& other);
		Client(int fd, const server* serverConfig);
		Client& operator=(const Client& other);
		void	processRequest();
		void	generateResponse();
		void	setRequestBuf(const string& buf) { requestBuf = buf; }
		string	getRequestBuf() const { return requestBuf; }
		void	updateState();
		void	printBuffer() const { cout << "Request Buffer: " << requestBuf << endl; }
		void	printRequest() const;
		void	printState() const;
		void	acceptConnection(int serverSocketIndex, int epollIndex);
		void	listenForEvents();
		int		handleClientData(int clientFd, int epollIndex);
		void	CreateSockets(const struct addrinfo& hint, struct addrinfo **newAddr, const std::string& port, const std::string& host);
		void	establishConnection(const Config& config);
		void	fileDescriptorDisposal();	
		void	makeConnection(int port, const std::string& host);
};

