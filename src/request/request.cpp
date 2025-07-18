#include "../../includes/request.hpp"

Request::Request() : method(UNKOWN), uri(""), version(""), servers(NULL), state(PARSINGREQUESTLINE) {}

Request::Request(const vector<server>* servers) : servers(servers), state(PARSINGREQUESTLINE) {}

bool    Request::validateURI() {
    if (uri.empty() || uri[0] != '/' || uri.find(' ') != string::npos)
        return false;
    for (size_t i = 0; i < uri.length(); ++i) {
        if (iscntrl(uri[i]) || isspace(uri[i]))
            return false;
    }
    return true;
}

bool    Request::validateVersion () {
    // here 5 is the length of the 3rd param
    if (version.compare(0, 5, "HTTP/"))
        return false;
    
    return true;
}

void    Request::processRequestLine(string& buffer) {
    size_t  pos = buffer.find(CRLF);
    if (pos == string::npos) return ;

    string line = buffer.substr(0, pos);
    buffer.erase(0, pos + CRLF_LENGTH);

    stringstream    ss(line);
    string          _method;

    ss >> _method >> this->uri >> this->version;
    if (ss.fail() || !ss.eof()) { this->state = PARSINGERROR; return; }

    if (_method == "GET") this->method = GET;
    else if (_method == "POST") this->method = POST;
    else if (_method == "DELETE") this->method = DELETE;
    else { this->method = UNKOWN; this->state = PARSINGERROR; return;}

    if (!validateURI() || !validateVersion())
        {this->state = PARSINGERROR; return;}

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