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
		string				resourceDir;
		ResourceType		resourceType;
		map<string, string>	headers;
		vector<char>		body;
		const location_b&	findLocationConfig();
		const string		generatefilename();
		vector<cgi_t>::const_iterator	searchExtensionHandler(const string& extension);

	public:
		Response& operator=(const Response& other);
		Response(const server* _serverConf, const Request& request);
		Response(const Response& _response);
		void	setHeader(string key, string value);
		int	prepareResponse();
		int	Post();
};

#endif