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
		Request(const vector<server>* servers);
		int		processRequestLine(string& buffer);
};
