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
		server			*_config;
		int				_clientFd;
		int				currentPort;
		string			requestBuf;
		string			responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			request;
		Response		response;
		unsigned short	httpStatusCode;

	public:

		Client(int fd, server* serverConfig);
		Client& operator=(const Client& other);
		void	processRequest();
		void	generateResponse();
		void	setRequestBuf(const string& buf) { requestBuf = buf; }
		string	getRequestBuf() const { return requestBuf; }
		string	getResponseBuf() const { return responseBuf; }
		ClientState getState() const { return state; }
		void	updateState();
		void	printBuffer() const { cout << "Request Buffer: " << requestBuf << endl; }
		void	printRequest() const;
		void	printState() const;
		int		handleData();
};

