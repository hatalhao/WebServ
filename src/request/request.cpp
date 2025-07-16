#include "../../includes/request.hpp"

Request::Request() : method(UNKOWN), uri(""), version("HTTP/1.1"), servers(NULL), state(PARSINGREQUESTLINE) {}

Request::Request(const vector<server>* servers) : servers(servers), state(PARSINGREQUESTLINE) {}

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
    this->state = PARSINGHEADERS;
    buffer.erase(0, pos + CRLF_LENGTH);
    return 1;
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
        if (colonPos == string::npos)
            return -1; // invalid header format
        string key = line.substr(0, colonPos);
        string value = line.substr(colonPos + 1);
        if (key.empty() || value.empty())
            return -1; // invalid header format
        headers[key] = value;
        buffer.erase(0, pos + CRLF_LENGTH);
    }
    return 0; // need more data to parse headers
}