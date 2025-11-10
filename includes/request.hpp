# pragma once

#include "origin.hpp"

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
		string							uri;
		string							path;
		string							queryString;
		string							version;
		map<string, string>				headers;
		map<string, string>				cookies;
		RequestState					state;

		bool	validateMethod(const string& _method, int& code);
		bool	validateUri(const string& uri, int &code);
		bool	validateVersion(int &code);
		bool    validateHeaderKey(string& key);
		bool	validateHeaderVal(const string& key, string& val);
		bool    validateHostHeader(const string& val);
		bool	validateCookieVal(const string& val);

	public:
		Request();
		~Request();
		string						getMethod() const { return method; }
		string						getHeader(const string& key) const;
		string						getPath() const { return path; }
		string						getQueryString() const { return queryString; }
		RequestState				getState() const { return state; }
		string						getVersion() const { return version; }
		const map<string, string>&	getHeaders() const { return headers; }
		int							processRequestLine(string& buffer);
		int							processHeaders(string& buffer);
		void						setPath(const string& _path) { path = _path; }
		string						getCookie(const string& name);
		map<string, string>&		getCookies() { return cookies; }
};
