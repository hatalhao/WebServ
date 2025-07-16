#include "../../includes/request.hpp"

Request::Request() : method(UNKOWN), uri(""), version(""), servers(NULL), state(PARSINGREQUESTLINE) {}

Request::Request(const vector<server>* servers) : servers(servers), state(PARSINGREQUESTLINE) {}


bool    Request::parseRequestLine(string& line) {
    stringstream    ss(line);
    string          _method;

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

void    Request::processRequestLine(string& buffer) {
    size_t  pos = buffer.find(CRLF);
    if (pos == string::npos)
        return ;
    string line = buffer.substr(0, pos);
    if (parseRequestLine(line) == false) {
        this->state = PARSINGERROR; return ; // we should return a status code here
    }
    buffer.erase(0, pos + CRLF_LENGTH);
    this->state = REQUESTLINE_READY;
}

int Request::processHeaders(string& buffer) {
    for (size_t pos; (pos = buffer.find(CRLF)) != string::npos;) {
        string line = buffer.substr(0, pos);
        if (line.empty()) {
            this->state = PARSINGCOMPLETE;
            buffer.erase(0, pos + CRLF_LENGTH);
            return 1; // headers parsing complete
        }
        size_t colonPos = line.find(':');
        if (colonPos == string::npos) {
            this->state = PARSINGERROR; return -1; // invalid headerformat
        }
        string key = line.substr(0, colonPos);
        string value = line.substr(colonPos + 1);
        if (key.empty() || value.empty()) {
            this->state = PARSINGERROR; return -1; // invalid headerformat
        }
        headers[key] = value;
        buffer.erase(0, pos + CRLF_LENGTH);
    }
    return 0; // need more data to parse headers
}