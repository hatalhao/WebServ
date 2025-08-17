#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "request.hpp"

enum ResourceType {
	IS_NOT_FOUND,
	IS_DIR,
	IS_FILE,
	IS_FORBIDDEN
};

class Response {
	protected:
		const server*		serverConf;
		const Request&		request;
		const location_b*	rules;
		string				resourcePath;
		ResourceType		resourceType;
		const location_b&	findLocationConfig();
		const string	generatefilename(string& basePath);

	public:
		Response(const server* _serverConf, const Request& request);
		int	prepareResponse();
		int	Post();
};

#endif