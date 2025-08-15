#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "request.hpp"

class Response {
	private:
		const server*		serverConf;
		const Request&		request;
		const location_b*	rules;
		string				resourcePath;
		const location_b&	findLocationConfig();

	public:
		Response(const server* _serverConf, const Request& request);
		int checkRequirements();
		int constructResourcePath();
};

#endif