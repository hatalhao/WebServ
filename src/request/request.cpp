#include "../../includes/request.hpp"

Request::Request() : method(UNKOWN), uri(""), version(""), servers(NULL), state(PARSINGREQUESTLINE) {}

Request::Request(const vector<server>* servers) : servers(servers), state(PARSINGREQUESTLINE) {}

bool    Request::validateURI() {
    if (uri.empty() || uri[0] != '/' || uri.find(' ') != NPOS)
        return false;
    for (size_t i = 0; i < uri.length(); ++i) {
        unsigned char c = uri[i];
        if (iscntrl(c) || isspace(c))
            return false;
    }
    return true;
}

bool    Request::validateVersion () {
    if (version.compare(0, HTTP_VERSION_STR_LEN, "HTTP/"))
        return false;
    
    string  versionNum = version.substr(HTTP_VERSION_STR_LEN);
    size_t dotPos = versionNum.find('.');

    if (dotPos == NPOS || dotPos == 0 || dotPos == versionNum.length())
        return false;
    
    if (versionNum.substr(0, dotPos).find_first_not_of("0123456789") != NPOS
        || versionNum.substr(dotPos + 1).find_first_not_of("0123456789") != NPOS)
    { 
        cerr << "BAD REQUEST LINE" << endl;
        return false;
    }

    return true;
}

void    Request::processRequestLine(string& buffer) {
    size_t  pos = buffer.find(CRLF);
    if (pos == NPOS) return ;

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

    this->state = REQUESTLINE_READY;
}

int toLower(int c) { return tolower(static_cast<unsigned char>(c)); }

bool    Request::validateHeaderKey(string& key) {
    if (key.empty())
        return false;
    for (size_t i = 0; i < key.length(); i++) {
        unsigned char c = key[i];
        if (!isalnum(c) && string(REQUEST_HEADER_EXTRA_CHARS).find(c) == string::npos)
            return false;
    }
    transform(key.begin(), key.end(), key.begin(), toLower);
    return true;
}

bool    Request::validateHeaderVal(const string& key, string& val) {
    size_t  i = val.find_first_not_of(" \t");
    if (i != string::npos) {
        size_t  j = val.find_last_not_of(" \t");
        val = val.substr(i, (j - i) + 1);
    }
    else
        val = "";
    if (key == "host") {
        for (size_t i = 0; i < val.length(); i++) {
            unsigned char c = val[i];
            if (!isalnum(c) && c != '-' && c != '.')
                return false;
        }
        if (val == "" || val[0] == '-' || val[0] == '.'
            || val[val.length() - 1] == '-')
            return false;
    }
    return true;
}

void    Request::processHeaders(string& buffer) {
    if (buffer.find(CRLF CRLF) == NPOS)
        return ;
    while (true) {
        size_t pos = buffer.find(CRLF);
        if (pos == NPOS) {this->state = PARSINGERROR; return;}

        string  line = buffer.substr(0, pos);
        buffer.erase(0, pos + CRLF_LENGTH);

        if (line.empty()) {
            if (this->headers.find("host") == this->headers.end())
                { this->state = PARSINGERROR; return; }
            this->state = REQUESTHEADERS_READY; return;
        }

        size_t  colPos = line.find(':');
        if (colPos == NPOS) {this->state = PARSINGERROR; return;}

        string  key = line.substr(0, colPos);
        if (!validateHeaderKey(key)) {this->state = PARSINGERROR; return;}

        string  val = line.substr(colPos + 1);
        if (!validateHeaderVal(key, val)) {this->state = PARSINGERROR; return;}

        this->headers[key] = val;
    }
}