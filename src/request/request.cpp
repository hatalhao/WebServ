#include "../../includes/request.hpp"

Request::Request(const Config* configuration) : state(PARSINGREQUESTLINE),  _config(configuration){}

bool    Request::validateMethod(const string& _method, int& code) {
    if (_method == "GET") this->method = GET;
    else if (_method == "POST") this->method = POST;
    else if (_method == "DELETE") this->method = DELETE;
    else if (_method == "PUT" || _method == "HEAD"
        || _method == "OPTIONS" || _method == "PATCH"
        || _method == "TRACE" || _method == "CONNECT")
            { this-> method = UNSUPPORTED; return (code = 501, false);}
    else { this->method = UNKOWN; return (code = 400, false);}

    return true;
}

bool    Request::validateURI(int &code) {
    if (uri.empty() || uri[0] != '/' || uri.find(' ') != NPOS)
        return (code = 400, false);
    for (size_t i = 0; i < uri.length(); ++i) {
        unsigned char c = uri[i];
        if (iscntrl(c) || isspace(c))
            return (code = 400, false);
    }
    return true;
}

bool    Request::validateVersion (int &code) {
    if (version.compare(0, HTTP_VERSION_STR_LEN, "HTTP/"))
        return (code = 400, false);
    
    string  versionNum = version.substr(HTTP_VERSION_STR_LEN);
    size_t dotPos = versionNum.find('.');

    if (dotPos == NPOS || dotPos == 0 || dotPos == versionNum.length())
        return (code = 400, false);
    
    if (versionNum.substr(0, dotPos).find_first_not_of("0123456789") != NPOS
        || versionNum.substr(dotPos + 1).find_first_not_of("0123456789") != NPOS)
        return (code = 400, false);

    if (versionNum != "1.1")
        return (code = 505, false);

    return true;
}

int Request::processRequestLine(string& buffer) {
    size_t  pos = buffer.find(CRLF);
    if (pos == NPOS) return 0;

    string line = buffer.substr(0, pos);
    int code = 0;

    size_t  methodEndPos = line.find(' ');
    if (methodEndPos == NPOS) { this->state = PARSINGERROR; return 400; }
    
    size_t uriEndPos = line.find(' ', methodEndPos + 1);
    if (uriEndPos == NPOS ) { this->state = PARSINGERROR; return 400; }
    
    if (line.find(' ', uriEndPos + 1) != NPOS)
        { this->state = PARSINGERROR; return 400; }

    this->uri = line.substr(methodEndPos + 1, uriEndPos - (methodEndPos + 1));
    this->version = line.substr(uriEndPos + 1);
    if (!validateMethod(line.substr(0, methodEndPos), code)
    || !validateURI(code) || !validateVersion(code))
        {this->state = PARSINGERROR; return code;}

    buffer.erase(0, pos + CRLF_LENGTH);
    this->state = REQUESTLINE_READY;
    return 0;
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

    for (size_t k = 0; k < val.length(); k++)
        if (iscntrl(val[k])) return false;

    if (key == "host") {
        if (val.empty() || val == "" || val[0] == '-'
            || val[0] == '.' || val[val.length() - 1] == '-')
            return false;
        for (size_t l = 0; l < val.length(); l++) {
            unsigned char c = val[l];
            if (!isalnum(c) && c != '-' && c != '.') return false;
        }
        if (val[val.length() - 1] == '.')
            val.erase(val.length() - 1);
    }
    else if (key == "content-length") {
        if (val.empty() || val == "")
            return false;
        for (size_t x = 0; x < val.length(); x++)
            if (!isalnum(val[x])) return false;
    }
    return true;
}

int Request::processHeaders(string& buffer) {
    if (buffer.find(CRLF CRLF) == NPOS)
        return 0;
    while (true) {
        size_t pos = buffer.find(CRLF);
        if (pos == NPOS) {this->state = PARSINGERROR; return 400;}

        string  line = buffer.substr(0, pos);
        
        if (line.empty()) {
            if (this->headers.find("host") == this->headers.end())
            { this->state = PARSINGERROR; return 400; }
            this->state = REQUESTHEADERS_READY; return 0;
        }
        
        size_t  colPos = line.find(':');
        if (colPos == NPOS) {this->state = PARSINGERROR; return 400;}
        
        string  key = line.substr(0, colPos);
        if (!validateHeaderKey(key)) {this->state = PARSINGERROR; return 400;}
        
        string  val = line.substr(colPos + 1);
        if (!validateHeaderVal(key, val)) {this->state = PARSINGERROR; return 400;}
        
        buffer.erase(0, pos + CRLF_LENGTH);
        this->headers[key] = val;
    }
    return 0;
}

int Request::processBody(string& buffer) {
    (void)buffer;
    return (0);
}