#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "origin.hpp"

enum ResourceType {
	IS_NOT_FOUND = 404,
	IS_DIR,
	IS_FILE,
	IS_FORBIDDEN = 403
};

class Response {
	private:
		Client*				client;
		const location_b*	rules;
		string				resourcePath;
		string				resourceExtension;
		string				resourceDir;
		ResourceType		resourceType;
		string				resolvedServerName;
		map<string, string>	headers;
		vector<char>		body;
		const location_b&	findLocationConfig();
		const string		generatefilename();
		string				getReasonPhrase();
		
		public:
		int					bodyFd;
		size_t				bodySize;
		size_t				bodySent;
		bool				isFileBody;
		Response& operator=(const Response& other);
		Response(Client* _client);
		Response(const Response& _response);
		void	setHeader(string key, string value);
		void	setBody(string& _body);
		size_t	getBodySize() { return body.size(); }
		int		prepareResponse();
		string	serializeHeaders();
		string	serializeHeadersAndBody();
		int	Post();
		int	Get();
		int	Delete();
		void	setErrorResponse(int code);
		bool	findHeader(string key) { return headers.find(key) != headers.end(); }
		
		string	searchCgiHandlerPath();
		string	getResourcePath() { return resourcePath; }
		string	getResourceExtension() { return resourceExtension; }
		string	getresourceDir() { return resourceDir; }
		const location_b*	getRules() { return rules; }
		
};

ResourceType	getResourceType(const char* resource);
#endif