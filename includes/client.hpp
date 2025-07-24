# pragma once

#include "origin.hpp"
#include "request.hpp"
#include "Config.hpp"
#include "response.hpp"

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
		const server	*_config;
		int             socketFd;
		string			requestBuf;
		string			responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			request;
		Response		response;
		unsigned short	httpStatusCode;

	public:
		Client(int fd, const server* serverConfig);
		Client();
		void	processRequest();
		void	generateResponse();
		void	setRequestBuf(const string& buf) { requestBuf = buf; }
		string	getRequestBuf() const { return requestBuf; }
		void	updateState();
		void	printBuffer() const { cout << "Request Buffer: " << requestBuf << endl; }
		void	printRequest() const;
		void	printState() const;
		void	establishConnection(int port, const std::string& host);
		void	makeConnection(int port, const std::string& host);
};

