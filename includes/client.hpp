# pragma once

#include "request.hpp"
#include "worker.hpp"
#include "response.hpp"
#include "manipulators.hpp"
#include "cgi.hpp"

#define KEEP_ALIVE_TIMEOUT 20 // 20 seconds

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


typedef struct	cgi_data {
	size_t	crlfPos;
	size_t	lfPos;
	string	_headers;
	string	_body;
	bool	headersFound;
	bool	isStatusSet;
	bool	sessionRequested;
} cgi_data;

class Client {
	private:
		Worker			*globalConfig;
		int				_clientFd;
		set<int>		condidateServersIndices;// Indexes of servers this client could belong to
		server*			serverConfig;
		// int				currentPort;
		string			requestBuf;
		string			responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			request;
		Response		response;
		string			cgiHandlerPath;
		cgi_data		cgiData;
		unsigned short	httpStatusCode;
		bool			keepAlive;		// Whether to keep connection alive
		time_t			lastActivity;	// Last activity timestamp for timeout
		void			resetWriteState();
		void			resetForNextRequest();	// Reset state for next request on same connection

	public:
    	size_t	bodySent;
		size_t	headerSent;
		char	fileBuffer[100000]; // Buffer for file streaming
		size_t	fileBufferUsed;   // How much of buffer is used
		size_t	fileBufferSent;   // How much of buffer has been sent
		size_t	fileReadOffset;   // Current file read position
		// Changed hostPort parameter to const reference to allow binding rvalues (e.g., std::make_pair)
		Client(int fd, Worker* _g_config, const pair<string, int>& hostPort);

		string					getRequestBuf() const { return requestBuf; }
		string					getResponseBuf() const { return responseBuf; }
		ClientState 			getState() const { return state; }
		Request&				getRequest() { return request; }
		Response&				getResponse()  { return response; }
		server*					getServer() { return serverConfig; }
		Worker*					getGlobalConfig() { return globalConfig; }
		const unsigned short&	getStatusCode() { return httpStatusCode; }
		bool					shouldKeepAlive() const { return keepAlive; }
		void					setKeepAlive(bool ka) { keepAlive = ka; }
		time_t					getLastActivity() const { return lastActivity; }
		void					updateActivity() { lastActivity = time(NULL); }
		void					resolveHost();
		void					processRequest();
		void					generateResponse();
		void					setRequestBuf(const string& buf) { requestBuf = buf; }
		void					updateState();
		// void					printBuffer() const { cout << "Request Buffer: " << requestBuf << endl; }
		// void					printRequest() const;
		// void					printState() const;
		int						handleReadEvent(int epollFd);
		int						handleWriteEvent();
		int						streamFileResponse(ssize_t& sent);
		void					setStatusCode(unsigned short status) { httpStatusCode = status; }
		string					getCgiHandlerPath() { return cgiHandlerPath; }
		void					onCgiDone(string& cgiOutput);
		void					onCgiError(unsigned short code);
		int						endStreamingFile(size_t n);
		void					forwardToCgi();
		int						parseCgiOutput(string& cgiOutput);
		void					checkSession(string& newSID, string& delSID);
};