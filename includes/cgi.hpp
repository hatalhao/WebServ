#pragma once

#include "response.hpp"

class CgiHandler : public Response {
	private:
		const string&		scriptPath;

		vector<string>		env_vars;
		int					pipeIn[2];
		int					pipeOut[2];
		pid_t				pid;
		void	setupEnv();

	public:
		CgiHandler(const Response& _response, const string& _scriptPath);
		void	execute();
};