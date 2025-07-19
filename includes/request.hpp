# pragma once

#include "Webserv.hpp"

#define REQUEST_HEADER_EXTRA_CHARS "!#$%&'*+-.^_`|~"
#define HTTP_VERSION_STR_LEN 5


typedef enum RequestState {
	PARSINGREQUESTLINE,
	REQUESTLINE_READY,
	PARSINGHEADERS,
	REQUESTHEADERS_READY,
	PARSINGBODY,
	PARSINGCOMPLETE,
	PARSINGERROR
} RequestState;


class Request {
	private:
		methods							method;
		string							uri;
		string							version;
		map<string, string>				headers;
		vector<char>					body;
		const vector<server>*			servers;
		RequestState					state;
		bool	validateMethod(const string& _method);
		bool	validateURI();
		bool	validateVersion();
		bool    validateHeaderKey(string& key);
		bool	validateHeaderVal(const string& key, string& val);

	public:
		Request();
		Request(const vector<server>* servers);
		methods	getMethod() const { return method; }
		string	getUri() const { return uri; }
		string	getVersion() const { return version; }
		const map<string, string>&	getHeaders() const { return headers; }
		void	processRequestLine(string& buffer);
		void	processHeaders(string& buffer);
		void	processBody(string& buffer);
		RequestState	getState() const { return state; }
		void	printMethod() const;
};
