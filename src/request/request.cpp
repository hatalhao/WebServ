#include "../../includes/request.hpp"
#include "../../includes/Config.hpp"

Request::Request(const server* serverConfig) : state(PARSINGREQUESTLINE),  conf(serverConfig){}

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
    if (key.empty()) return false;

    for (size_t i = 0; i < key.length(); i++) {
        unsigned char c = key[i];
        if (!isalnum(c) && string(REQUEST_HEADER_EXTRA_CHARS).find(c) == string::npos)
            return false;
    }
    transform(key.begin(), key.end(), key.begin(), toLower);
    if (key == "content-length" && ((headers.find(key) != headers.end())
                    || headers.find("transfer-encoding") != headers.end()))
        return false;
    if (key == "transfer-encoding" && ((headers.find(key) != headers.end())
                    || headers.find("content-length") != headers.end()))
        return false;

    if (key == "host" && this->headers.find(key) != this->headers.end())
        return false;
    return true;
}

bool    Request::validateHostHeader(string& val) {
    if (val.empty() || val[0] == '.') return false;

    size_t colonPos = val.find(':');
    size_t hostname_end = (colonPos != std::string::npos) ? colonPos : val.length();

    // validating the port
    if (colonPos != std::string::npos) {
        if (colonPos == val.length() - 1) return false;
        for (size_t i = colonPos + 1; i < val.length(); ++i)
            if (!isdigit(val[i])) return false;
    }

    if (hostname_end > 0 && val[hostname_end - 1] == '.')
        --hostname_end;

    if (hostname_end == 0 || val[0] == '-' || val[hostname_end - 1] == '-')
        return false;

    for (size_t i = 0; i < hostname_end; ++i) {
        if (!isalnum(val[i]) && val[i] != '-' && val[i] != '.') return false;
        if (val[i] == '.' && (i == 0 || val[i - 1] == '.' || val[i - 1] == '-'))
            return false;
        else if (val[i] == '-' && i > 0 && val[i - 1] == '.') return false;
    }

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

    if (key == "host" && !validateHostHeader(val))
        return false;
    else if (key == "content-length") {
        if (val.empty()) return false;
        for (size_t x = 0; x < val.length(); x++)
            if (!isdigit(val[x])) return false;
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
            buffer.erase(0, pos + CRLF_LENGTH);
            this->state = REQUESTHEADERS_READY; return 0;
        }

        size_t  colPos = line.find(':');
        if (colPos == NPOS) {this->state = PARSINGERROR; DEBUG cout << "[" << line << "]\n"; return 400;}
        
        string  key = line.substr(0, colPos);
        if (!validateHeaderKey(key)) {this->state = PARSINGERROR; return 400;}
        
        string  val = line.substr(colPos + 1);
        if (!validateHeaderVal(key, val)) {this->state = PARSINGERROR; return 400;}

        if (key == "transfer-encoding" && (val.empty() || val != "chunked")) return 501;
    
        buffer.erase(0, pos + CRLF_LENGTH);
        this->headers[key] = val;
    }
    return 0;
}

int Request::processBody(string& buffer) {
    size_t  contentLen;

    if (headers.find("content-length") != headers.end()) {
        contentLen = atoll(headers["content-length"].c_str());
        if (contentLen > this->conf->maxBodySize)
            return (this->state = PARSINGERROR, 413);
        if (buffer.length() < contentLen)
            return 0;
        this->body.assign(buffer.begin(), buffer.begin() + contentLen);
        buffer.erase(0, contentLen);
        this->state = PARSINGCOMPLETE;
    }
    else if (headers.find("transfer-encoding") != headers.end()) {
        while (true) {
            size_t crlf_pos = buffer.find(CRLF);
            if (crlf_pos == string::npos) return 0;

            stringstream ss(buffer.substr(0, crlf_pos));
            size_t chunkSize;
            ss >> std::hex >> chunkSize;

            if (ss.fail()) return(this->state = PARSINGERROR, 400);

            if (this->body.size() + chunkSize > this->conf->maxBodySize)
                return (this->state = PARSINGERROR, 413);

            size_t chunkDataStart = crlf_pos + CRLF_LENGTH;
            size_t totalChunkBlockSize = chunkDataStart + chunkSize + CRLF_LENGTH;
            if (buffer.length() < totalChunkBlockSize) return 0;

            if (chunkSize == 0) {
                buffer.erase(0, totalChunkBlockSize - chunkSize);
                return (this->state = PARSINGCOMPLETE, 0); 
            }
            body.insert(body.end(), buffer.begin() + chunkDataStart, buffer.begin() + chunkDataStart + chunkSize);
            buffer.erase(0, totalChunkBlockSize);
        }
    }
    else
        return 400;
    return (0);
}
