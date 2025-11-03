#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "origin.hpp"
#include "Session.hpp"

enum ResourceType {
	IS_NOT_FOUND = 404,
	IS_DIR,
	IS_FILE,
	IS_FORBIDDEN = 403
};

typedef struct ResourceInfo {
	string			path;
	string			extension;
	string			pathDir;
	ResourceType	type;
	off_t			size;
} file_info;

class Response {
	private:
		Client*				client;
		const location_b*	rules;
		Session*			session;
		file_info			resourceInfo;
		bool                isCgiRequest;
		string				resolvedServerName;
		map<string, string>	headers;
		vector<char>		body;
		const location_b&	findLocationConfig(const string& path);
		bool				detectRedirectLoop(const string& startPath, int maxDepth, set<string>& visitedPaths);
		string				resolveRedirect(const string& redirectTarget, const string& locationName);
		string				getReasonPhrase();

	public:
		Response(Client* _client);
		Response(const Response& _response);
		int					bodyFd;
		size_t				bodySize;
		size_t				bodySent;
		bool				isFileBody;
		void				setHeader(string key, string value);
		void				setBody(string& _body);
		size_t				getBodySize() { return body.size(); }
		int					prepareResponse();
		string				serializeHeaders();
		string				serializeHeadersAndBody();
		int					Post();
		int					Get();
		int					Delete();
		void				setErrorResponse(int code);
		bool				findHeader(string key) { return headers.find(key) != headers.end(); }

		string				searchCgiHandlerPath();
		string				getResourcePath() { return resourceInfo.path; }
		string				getResourceExtension() { return resourceInfo.extension; }
		string				getresourceDir() { return resourceInfo.pathDir; }
		const location_b*	getRules() { return rules; }
		void				resolveResourcePath(const Request& request, bool isPOST);
		bool				checkRedirections();
		void				handleIndexFile();
		bool				checkCgiRequest() { return isCgiRequest; }
		int					prepareResource(const Request& request);

};

ResourceType	getResourceType(file_info& resource);
#endif