#include "../includes/request.hpp"
#include "../includes/client.hpp"

Request::Request(Client* _client) : client(_client), state(PARSINGREQUESTLINE){}

static int hexVal(unsigned char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    return -1;
}

static bool decodeComponent(const std::string& in, std::string& out, bool plusAsSpace) {
    out.clear();
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(in[i]);
        if (c == '%') {
            if (i + 2 >= in.size()) return false;
            int hi = hexVal(static_cast<unsigned char>(in[i + 1]));
            int lo = hexVal(static_cast<unsigned char>(in[i + 2]));
            if (hi < 0 || lo < 0) return false; // invalid hex
            unsigned char dec = static_cast<unsigned char>((hi << 4) | lo);
            if (iscntrl(dec)) return false;
            out.push_back(static_cast<char>(dec));
            i += 2;
        } else if (plusAsSpace && c == '+') {
            out.push_back(' ');
        } else {
            if (iscntrl(c)) return false; 
            out.push_back(static_cast<char>(c));
        }
    }
    return true;
}

bool    Request::validateMethod(const string& _method, int& code) {
    if (_method == "GET" || _method == "POST" || _method == "DELETE")
        this->method = _method;
    else if (_method == "PUT" || _method == "HEAD"
        || _method == "OPTIONS" || _method == "PATCH"
        || _method == "TRACE" || _method == "CONNECT")
            return (code = 501, false);
    else
        return (code = 400, false);
    return true;
}

bool    Request::validateUri(const string& uri, int &code) {
    if (uri.empty() || uri[0] != '/')
        return (code = 400, false);
    for (size_t i = 0; i < uri.length(); ++i) {
        unsigned char c = uri[i];
        if (iscntrl(c))
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

bool    Request::validateHostHeader(const string& val) {
    if (val.empty() || val[0] == '.') return false;
    
    size_t colonPos = val.find(':');
    size_t hostname_end = (colonPos != std::string::npos) ? colonPos : val.length();

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

static string   trim(const string& s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, (end - start) + 1);
}

bool Request::validateCookieVal(const string& val) {
    this->cookies.clear();
    
    std::stringstream ss(val);
    std::string cookie_pair;

    while (getline(ss, cookie_pair, ';')) {
        size_t equals_pos = cookie_pair.find('=');

        if (equals_pos == string::npos) {
            if (trim(cookie_pair).empty())
                continue;
            return false;
        }
        
        string key = trim(cookie_pair.substr(0, equals_pos));
        string value = trim(cookie_pair.substr(equals_pos + 1));

        if (key.empty())
            return false;

        this->cookies[strToLower(key)] = value;
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
    if (key == "cookie" && !validateCookieVal(val))
        return false;
    else if (key == "content-length") {
        if (val.empty()) return false;
        for (size_t x = 0; x < val.length(); x++)
        if (!isdigit(val[x])) return false;
    }
    
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

    string uri = line.substr(methodEndPos + 1, uriEndPos - (methodEndPos + 1));
    uri = uri.substr(0, uri.find('#'));
    size_t queryPos = uri.find('?');
    string rawPath = (queryPos != NPOS) ? uri.substr(0, queryPos) : uri;
    string rawQuery = (queryPos != NPOS) ? uri.substr(queryPos + 1) : string();

    string decodedPath;
    string decodedQuery;
    if (!decodeComponent(rawPath, decodedPath, false)) { this->state = PARSINGERROR; return 400; }
    if (!rawQuery.empty() && !decodeComponent(rawQuery, decodedQuery, true)) { this->state = PARSINGERROR; return 400; }

    this->path = decodedPath;
    this->queryString = decodedQuery;
    this->version = line.substr(uriEndPos + 1);
    if (!validateMethod(line.substr(0, methodEndPos), code)
    || !validateUri(this->path, code) || !validateVersion(code))
        {this->state = PARSINGERROR; return code;}

    buffer.erase(0, pos + CRLF_LENGTH);
    this->state = REQUESTLINE_READY;
    return 0;
}

string Request::getHeader(const string& key) const {
    map<string, string>::const_iterator it = headers.find(key);
    return (it != headers.end() ? it->second : "");
}

int Request::processHeaders(string& buffer) {
    while (true) {
        size_t pos = buffer.find(CRLF);
        if (pos == NPOS) { return 0; }

        string  line = buffer.substr(0, pos);
        
        if (line.empty()) {
            if (this->headers.find("host") == this->headers.end())
            { this->state = PARSINGERROR; return 400; }
            buffer.erase(0, pos + CRLF_LENGTH);
            this->state = REQUESTHEADERS_READY; return 0;
        }

        size_t  colPos = line.find(':');
        if (colPos == NPOS) {this->state = PARSINGERROR; return 400;}
        
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

    size_t  maxBodySize = this->client->getServer()->maxBodySize;

    if (headers.find("content-length") != headers.end()) {
        contentLen = atoll(headers["content-length"].c_str());
        if (contentLen > maxBodySize)
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

            if (this->body.size() + chunkSize > maxBodySize)
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
        return (this->state = PARSINGERROR, 400);
    return (0);
}


string  Request::getCookie(const string& name) {
    map<string, string>::const_iterator it = cookies.find(name);
    return (it != cookies.end() ? it->second : "");   
}