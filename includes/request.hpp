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
		string							method;
		string							path;
		string							queryString;
		string							version;
		map<string, string>				headers;
		vector<char>					body;
		RequestState					state;
		const size_t&					_maxBodySize;

		bool	validateMethod(const string& _method, int& code);
		bool	validateURI(int &code);
		bool	validateVersion(int &code);
		bool    validateHeaderKey(string& key);
		bool	validateHeaderVal(const string& key, string& val);
		bool    validateHostHeader(string& val);

	public:
		Request(const size_t& maxBodySize);
		string	getMethod() const { return method; }
		string	getPath() const { return path; }
		string	getQueryString() const { return queryString; }
		const vector<char>&	getBody() const { return body; }
		string	getVersion() const { return version; }
		const map<string, string>&	getHeaders() const { return headers; }
		int	processRequestLine(string& buffer);
		int	processHeaders(string& buffer);
		int	processBody(string& buffer);
		RequestState	getState() const { return state; }
};
