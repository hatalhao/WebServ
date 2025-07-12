#ifndef HTTP_HPP
#define HTTP_HPP

#include "webserv.hpp"

typedef enum status {
    PARSINGREQUESTLINE,
    PARSINGHEADERS,
    PARSINGBODY,
    PARSINGCOMPLETE,
    PARSINGERROR
} status;

typedef enum methods {
    GET,
    POST,
    DELETE,
    UNKOWN
}  methods;

class Request {
    private:
        std::vector<u_int8_t>               buffer;
        methods                             method;
        std::string                         uri;
        float                               version;
        std::map<std::string, std::string>  headers;
        std::vector<char>                   body;
        const std::vector<server>*          servers;

    public:
        Request requestParser(const std::vector<server>* servers);

};

#endif