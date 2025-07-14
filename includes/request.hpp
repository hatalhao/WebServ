#ifndef HTTP_HPP
#define HTTP_HPP

#include "client.hpp"

typedef enum RequestState {
	PARSINGREQUESTLINE,
	PARSINGHEADERS,
	PARSINGBODY,
	PARSINGCOMPLETE,
	PARSINGERROR
};


typedef struct Request {
	std::vector<u_int8_t>               buffer;
	methods                             method;
	std::string                         uri;
	float                               version;
	std::map<std::string, std::string>  headers;
	std::vector<char>                   body;
	const std::vector<server>*          servers;
	RequestState                        state;
} Request;

#endif