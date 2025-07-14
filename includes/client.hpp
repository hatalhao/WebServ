#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "config.hpp"

enum ClientState {
	READING_REQUEST_LINE,
	READING_HEADERS,
	READING_BODY,
	GENERATING_RESPONSE,
	SENDING_RESPONSE
};

typedef enum methods {
    GET,
    POST,
    DELETE,
    UNKOWN
};

class Client {
	private:
		int             socketFd;
		std::string     requestBuf;
		std::string     responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			Request;

	public:
		void	processRequest();

};

#endif