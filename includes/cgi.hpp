#pragma once

#include "response.hpp"

class CgiHandler : public Response {
	private:
		const string&		scriptPath;

		char				**envp;
		int					pipeIn[2];
		int					pipeOut[2];
		pid_t				pid;
		void	setupEnv();
		void	executeChild();
		void	executeParent();
		void	populateResponse(const string& output);

	public:
		CgiHandler(const Response& _response, const string& _scriptPath);
		void		execute();
		Response	exportResponse();
};

template <typename T>
string toString(T x) {
	stringstream ss;
	ss << x;
	return ss.str();
}