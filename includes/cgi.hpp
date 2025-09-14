#pragma once

#include "response.hpp"
#include "client.hpp"
#include "cookies.hpp"

enum CgiState {
	CGI_IDLE,
	CGI_READING,
	CGI_WRITING,
	CGI_DONE
};

class CgiHandler {
	private:
		Client&				_client;
		char				**envp;
		pid_t				pid;
		CgiState			state;
		int					writePipeFd;
		int					readPipeFd;
		string				output;
		size_t				bytesRecieved;
		string				sessionId;
		bool				sessionCreated;

		void	setupEnv();
		void	executeChild(int *pipeIn, int *pipeOut);
		void	addSessionToEnv(vector<string>& env_vars);
		string	createNewSession(); 

	public:
		CgiHandler(Client& client);
		bool		start();
		CgiState	getState() { return state; }
		int	getWriteFd() const { return writePipeFd; }
		int getReadFd() const { return readPipeFd; }
		void	manageReadEvent(int epollFd);
		void	manageWriteEvent(int epollFd);
		string	getSessionId() const { return sessionId; }
		bool	wasSessionCreated() const { return sessionCreated; }
};

template <typename T>
string toString(T x) {
	stringstream ss;
	ss << x;
	return ss.str();
}