#include "../../includes/response.hpp"
#include "../../includes/cgi.hpp"

Response::Response(const server* _serverConf, Request& _request, unsigned short& _statusCode) :
        serverConf(_serverConf) , request(_request), rules(NULL), statusCode(_statusCode),
		bodyFd(-1), bodySize(0), bodySent(0), isFileBody(false){}

Response::Response(const Response& other) :
	serverConf(other.serverConf), request(other.request), rules(other.rules),
	resourcePath(other.resourcePath), resourceDir(other.resourceDir),
	resourceType(other.resourceType), headers(other.headers), body(other.body),
	statusCode(other.statusCode){}

const location_b&	Response::findLocationConfig() {
	const location_b *best = NULL, *root = NULL;
	size_t	bestLen = 0;	
	for (vector<location_b>::const_iterator it = this->serverConf->locations.begin();
		it != this->serverConf->locations.end(); it++)
	{
		if (it->name == "/")
			root = &(*it);
		if (this->request.getPath().find(it->name) == 0) {
			if(bestLen < it->name.length() || !best) {
				bestLen = it->name.length();
				best = &(*it);
			}
		}
	}
	if (!best)
		best = root;
	return *best;
}

Response& Response::operator=(const Response& other) {
	if (this != &other) {
		this->serverConf = other.serverConf;
		this->request = other.request;
		this->rules = other.rules;
		this->resourcePath = other.resourcePath;
		this->resourceDir = other.resourceDir;
		this->resourceType = other.resourceType;
		this->resolvedServerName = other.resolvedServerName;
		this->headers = other.headers;
		this->body = other.body;
		this->statusCode = other.statusCode;
	}
	return *this;
}

ResourceType	getResourceType(const char* resource) {
	struct stat statBuf;
	
	if (stat(resource, &statBuf) != 0)
		return (errno == ENOENT) ? IS_NOT_FOUND : IS_FORBIDDEN;
	else if (S_ISDIR(statBuf.st_mode)) return IS_DIR;
	else if (S_ISREG(statBuf.st_mode)) return IS_FILE;

	return IS_FORBIDDEN;
}

int Response::prepareResponse() {
    this->rules = &(this->findLocationConfig());
    /**
	 * check for redirections here
	 * 
	 * if (rules.returnCode != 0) {
	 * 	if (rules.returnCode >= 300 && rules.returnCode < 400) {
	 * 		// add Location Header
	 * 	}
	 * 	return rules.returnCode;
	 * }
	**/
	if (std::find(rules->methods.begin(),
				rules->methods.end(),
				this->request.getMethod()) == rules->methods.end())
		return (this->statusCode = 405, -1);

	this->resourceDir = this->rules->path.empty() ? this->serverConf->root : this->rules->path;
	this->resourcePath = this->resourceDir + this->request.getPath();

	this->resourceDir += this->rules->name;

	size_t pos;
	while ((pos = this->resourcePath.find("//")) != string::npos)
		this->resourcePath.replace(pos, 2, "/");

	size_t dotPos = this->request.getPath().find_last_of(".");
	if (dotPos != string::npos) {
		this->resourceExtension = this->request.getPath().substr(dotPos);
	} else {
		this->resourceExtension = "";
	}

	this->resourceType = getResourceType(this->resourcePath.c_str());
	if (this->resourceType == IS_NOT_FOUND || this->resourceType == IS_FORBIDDEN)
		return (this->statusCode = this->resourceType, -1);

	return (0);
}

string Response::serializeHeaders() {
    stringstream ss;

    ss << "HTTP/1.1 " << statusCode << " " << getReasonPhrase() << CRLF;

    ss << "Server: Webserv/1.0" << CRLF;
    ss << "Connection: close" << CRLF;

    for (map<string,string>::iterator it = headers.begin(); it != headers.end(); ++it)
        ss << it->first << ": " << it->second << CRLF;

    ss << CRLF;
    return ss.str();
}

string Response::serializeHeadersAndBody() {
    stringstream ss;
    ss << serializeHeaders();
    if (!body.empty())
        ss.write(body.data(), body.size());
    return ss.str();
}


const string	Response::generatefilename() {
	stringstream ss;
	ss << "file_" << time(NULL) << ".upload";
	return ss.str();
}

string Response::getReasonPhrase() {
	switch (this->statusCode) {
		case 200: return "OK";
		case 201: return "Created";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 413: return "Request Entity Too Large";
		case 414: return "Request-URI Too Long";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		default: return "OK";
	}
}

void Response::setErrorResponse(int code) {
	this->statusCode = code;
	this->rules = &(this->findLocationConfig());

	map<int, string>::const_iterator it = this->serverConf->errorPageMap.find(code);
	string content;
	if (it != this->serverConf->errorPageMap.end()) {
		ifstream file(it->second.c_str());
		if (file.is_open()) {
			content.assign((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
			file.close();
		}
	}
	if (content.empty()) {
		// Default error message
		stringstream ss;
		ss << "<html><body><h1>Error " << code << "</h1><p>" << getReasonPhrase() << "</p></body></html>";
		content = ss.str();
	}
	this->setBody(content);
	this->setHeader("Content-Type", "text/html");
	stringstream len_ss;
	len_ss << content.length();
	this->setHeader("Content-Length", len_ss.str());
}

void	Response::setHeader(string key, string value)
	{ this->headers.insert(pair<string, string>(key, value)); }

void	Response::setBody(string& _body)
{
	this->body.assign(_body.begin(), _body.end());
	this->bodySize = this->body.size();
}

string	Response::searchCgiHandlerPath() {
	vector<cgi_t>::const_iterator it = this->rules->cgiList.begin();
	for (; it != this->rules->cgiList.end(); it++) {
		if (it->extension == this->resourceExtension)
				return it->path;
	}
	return "";
}