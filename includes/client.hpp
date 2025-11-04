# pragma once

#include "request.hpp"
#include "worker.hpp"
#include "response.hpp"
#include "manipulators.hpp"
#include "cgi.hpp"

#define KEEP_ALIVE_TIMEOUT 20

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
		set<int>		condidateServersIndices;
		server*			serverConfig;
		string			requestBuf;
		string			responseBuf;
		size_t          contentLen;
		ClientState     state;
		Request			request;
		Response		response;
		string			cgiHandlerPath;
		cgi_data		cgiData;
		unsigned short	httpStatusCode;
		bool			keepAlive;
		time_t			lastActivity;
		void			resetWriteState();
		void			resetForNextRequest();

	public:
    	size_t	bodySent;
		size_t	headerSent;
		char	fileBuffer[100000];
		size_t	fileBufferUsed; 
		size_t	fileBufferSent;
		size_t	fileReadOffset;

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