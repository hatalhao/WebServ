# pragma once

#include "origin.hpp"
#include "request.hpp"

enum ClientState {
	READING_REQUEST_LINE,
	READING_HEADERS,
	READING_BODY,
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

	public:
		Client(int fd);
		void	processRequest();
		void	setRequestBuf(const string& buf) { requestBuf = buf; }
		void	printRequest() const;
		void	printBuffer() const { cout << "Request Buffer: " << requestBuf << endl; }
		void	printState() const;
};

