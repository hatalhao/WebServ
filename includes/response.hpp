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
		Request&		request;
		const location_b*	rules;
		string				resourcePath;
		string				resourceDir;
		ResourceType		resourceType;
		string				resolvedServerName;
		map<string, string>	headers;
		vector<char>		body;
		unsigned short		&statusCode;
		const location_b&	findLocationConfig();
		const string		generatefilename();
		vector<cgi_t>::const_iterator	searchExtensionHandler(const string& extension);

	public:
		Response& operator=(const Response& other);
		Response(const server* _serverConf, Request& request, unsigned short& statusCode);
		Response(const Response& _response);
		void	setHeader(string key, string value);
		void	setBody(string& _body);
		int	prepareResponse();
		int	Post();
};

#endif