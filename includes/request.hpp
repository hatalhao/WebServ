# pragma once

#include "Webserv.hpp"

typedef enum RequestState {
	PARSINGREQUESTLINE,
	PARSINGHEADERS,
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
		bool	parseRequestLine(string& line);

	public:
		Request();
		Request(const vector<server>* servers);
		methods	getMethod() const { return method; }
		string	getUri() const { return uri; }
		string	getVersion() const { return version; }
		const map<string, string>&	getHeaders() const { return headers; }
		int		processRequestLine(string& buffer);
		int		processHeaders(string& buffer);
		RequestState	getState() const { return state; }
};
