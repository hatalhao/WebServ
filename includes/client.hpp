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
		void	printRequest() const {
			cout << "Request Method: " << request.getMethod() << endl;
			cout << "Request URI: " << request.getUri() << endl;
			cout << "Request Version: " << request.getVersion() << endl;
			for (map<string, string>::const_iterator it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it) {
				cout << it->first << ": " << it->second << endl;
			}
		}
};

