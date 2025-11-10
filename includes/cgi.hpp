#pragma once

#include "response.hpp"
#include "client.hpp"
#include "Session.hpp"

enum CgiState {
	CGI_IDLE,
	CGI_READING,
	CGI_WRITING,
	CGI_DONE
};

#define CGI_TIMEOUT_SECONDS 10

class CgiHandler {
	private:
		Client&				_client;
		char				**envp;
		pid_t				pid;
		CgiState			state;
		int					writePipeFd;
		int					readPipeFd;
		string				output;
		size_t				bodyBytesWritten;
		size_t				expectedBodySize;
		string				sessionId;
		bool				sessionCreated;
		time_t				startTime;

		void	setupEnv();
		void	executeChild(int *pipeIn, int *pipeOut);
		void	addSessionToEnv(vector<string>& env_vars);
		string	createNewSession(); 

	public:
		CgiHandler(Client& client);
		~CgiHandler();
		CgiState	getState() { return state; }
		int			getWriteFd() const { return writePipeFd; }
		int 		getReadFd() const { return readPipeFd; }
		string		getSessionId() const { return sessionId; }
		time_t		getStartTime() const { return startTime; }
		pid_t		getPid() const { return pid; }
		Client&		getClient() { return _client; }
		void		setState(CgiState _state) { state = _state; }
		bool		start();
		void		manageReadEvent(int epollFd);
		void		manageWriteEvent(int epollFd);
		bool		wasSessionCreated() const { return sessionCreated; }
};

template <typename T>
string toString(T x) {
	stringstream ss;
	ss << x;
	return ss.str();
}