# pragma once

#include "origin.hpp"
#include "request.hpp"
#include "Config.hpp"

enum ClientState {
	READING_REQUEST_LINE,
	READING_HEADERS,
	READING_BODY,
	READING_DONE,
	READING_BAD_REQUEST,
	GENERATING_RESPONSE,
	SENDING_RESPONSE
};


class Client {
	private:
		int             socketFd;
		string			requestBuf;
		string			responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			request;
		const server	*_config;
		unsigned short	httpStatusCode;

	public:
		Client(int fd, const server* serverConfig);
		Client();
		void	processRequest();
		void	setRequestBuf(const string& buf) { requestBuf = buf; }
		string	getRequestBuf() const { return requestBuf; }
		void	printRequest() const;
		void	printBuffer() const { cout << "Request Buffer: " << requestBuf << endl; }
		void	updateState();
		void	printState() const;
		void	establishConnection(int port, const std::string& host);
		void	makeConnection(int port, const std::string& host);
};

