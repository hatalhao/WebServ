#include "../../includes/request.hpp"

bool    Request::parseRequestLine(string& line) {
    stringstream    ss(line);
    string          _method;


    (void)this->state;
    (void)this->servers;
    ss >> _method >> this->uri >> this->version;
    if (ss.fail() || !ss.eof())
        return false;
    if (_method == "GET") this->method = GET;
    else if (_method == "POST") this->method = POST;
    else if (_method == "DELETE") this->method = DELETE;
    else { this->method = UNKOWN; return false; } // status code here (unsupported method)

    if (uri.empty() || uri[0] != '/' || uri.find(' ') != string::npos)
        return false ; // status code here
    
    if (version != "HTTP/1.1")
        return false; // status code here (unsupported http version)

    return true;
}

int Request::processRequestLine(string& buffer) {
    size_t  pos = buffer.find(CRLF);
    if (pos == string::npos)
        return 0;
    string line = buffer.substr(0, pos);
    if (parseRequestLine(line) == false)
        return -1; // we should return a status code here
    return 1;
}