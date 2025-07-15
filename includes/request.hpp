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
		Request() : method(UNKOWN), uri(""), version("HTTP/1.1"), servers(NULL), state(PARSINGREQUESTLINE) {}
		Request(const vector<server>* servers) : servers(servers), state(PARSINGREQUESTLINE) {}
		int		processRequestLine(string& buffer);
};
