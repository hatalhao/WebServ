#pragma once

#include "response.hpp"
#include "client.hpp"

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

		void	setupEnv();
		void	executeChild(int *pipeIn, int *pipeOut); 
		void	populateResponse(const string& output);

	public:
		CgiHandler(Client& client);
		bool		start();
		CgiState	getState() { return state; }
		int	getWriteFd() const { return writePipeFd; }
		int getReadFd() const { return readPipeFd; }
		void	manageReadEvent(int epollFd);
		void	manageWriteEvent(int epollFd);
		Response	exportResponse();
};

template <typename T>
string toString(T x) {
	stringstream ss;
	ss << x;
	return ss.str();
}