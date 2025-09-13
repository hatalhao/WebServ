# pragma once

#include "request.hpp"
#include "Config.hpp"
#include "response.hpp"
#include "manipulators.hpp"
#include "cgi.hpp"

enum ClientState {
	READING_REQUEST_LINE,
	READING_HEADERS,
	READING_BODY,
	READING_DONE,
	READING_BAD_REQUEST,
	WAITING_FOR_CGI,
	GENERATING_RESPONSE,
	SENDING_RESPONSE
};


#define MAXEVENTS 1024
#define FINISHED 0
#define DISCONNECTED -1
#define CONTINUE 1

class Client {
	private:
		Config			*globalConfig;
		int				_clientFd;
		server*			serverConfig;
		int				currentPort;
		string			requestBuf;
		string			responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			request;
		Response		response;
		string			cgiHandlerPath;
		unsigned short	httpStatusCode;
		void    resetWriteState();
		
	public:
    	size_t bodySent;
		size_t headerSent;
		char fileBuffer[100000]; // Buffer for file streaming
		size_t fileBufferUsed;   // How much of buffer is used
		size_t fileBufferSent;   // How much of buffer has been sent
		size_t fileReadOffset;   // Current file read position
		Client(int fd, Config* _g_config);
	
		string		getRequestBuf() const { return requestBuf; }
		string		getResponseBuf() const { return responseBuf; }
		ClientState getState() const { return state; }
		Request&	getRequest() { return request; } 
		Response&	getResponse()  { return response; }
		server*		getServer() { return serverConfig; }
		const unsigned short&	getStatusCode() { return httpStatusCode; }
		void	resolveHost();
		
		void	processRequest();
		void	generateResponse();
		void	setRequestBuf(const string& buf) { requestBuf = buf; }
		void	updateState();
		void	printBuffer() const { cout << "Request Buffer: " << requestBuf << endl; }
		void	printRequest() const;
		void	printState() const;
		int		handleReadEvent(int epollFd);
		int		handleWriteEvent();
	
		void	setStatusCode(unsigned short status) { httpStatusCode = status; }
		string	getCgiHandlerPath() { return cgiHandlerPath; }
		void	onCgiDone(string& cgiOutput);
		void	onCgiError();
};

