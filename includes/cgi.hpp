#pragma once

#include "response.hpp"

class CgiHandler {
	private:
		const Response&		response;
		const Request&      request;
		const location_b&   route;
		const string&		scriptPath;

		char**              env;
		int					pipeIn[2];
		int					pipeOut[2];
		pid_t				pid;

	public:
		CgiHandler(const Response& response, const cgi_t& _cgiConfig);
		~CgiHandler();
		void	execute();
};