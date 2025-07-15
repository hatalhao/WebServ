#ifndef CLIENT_HPP
#define CLIENT_HPP

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
		void	processRequest();

};

#endif