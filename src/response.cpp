#include "../includes/response.hpp"
#include "../includes/client.hpp"

Response::Response(Client* _client) :
        client(_client) , rules(NULL), session(NULL), isCgiRequest(false),
		bodyFd(-1), bodySize(0), bodySent(0), isFileBody(false) {}

Response::Response(const Response& other) :
	client(other.client), rules(other.rules),
	resourceInfo(other.resourceInfo), headers(other.headers), body(other.body){}

const string	getFileExtension(const string& filename) {
	size_t dotPos = filename.find_last_of(".");
	if (dotPos != string::npos)
		return filename.substr(dotPos);
	return "";
}

const location_b& Response::findLocationConfig(const string& path) {
    const location_b* bestMatch = NULL;
    const location_b* rootLocation = NULL;
    size_t longestMatchLen = 0;
    string normalizedPath(path);

    for (vector<location_b>::const_iterator it = client->getServer()->locations.begin();
         it != client->getServer()->locations.end(); ++it) {
        
        string locName = it->name;

        if (locName.empty() || locName[0] != '/')
            locName.insert(0, "/");

        if (locName == "/") {
            rootLocation = &(*it);
            continue;
        }

        if (locName[locName.length() - 1] != '/')
            locName += '/';
        
        size_t currentMatchLen = locName.length();

        if (normalizedPath.find(locName) == 0) {
            if (currentMatchLen > longestMatchLen) {
                longestMatchLen = currentMatchLen;
                bestMatch = &(*it);
            }
        } 

        else if (normalizedPath + "/" == locName) {
            if (currentMatchLen > longestMatchLen) {
                longestMatchLen = currentMatchLen;
                bestMatch = &(*it);
            }
        }
    }
    if (bestMatch == NULL)
        bestMatch = rootLocation; 
    
    return *bestMatch;
}

string Response::resolveRedirect(const string& redirectTarget, const string& locationName) {
    string resolvedPath;

    if (redirectTarget.empty() || redirectTarget[0] == '/')
        resolvedPath = redirectTarget;
    else {
        string locName(locationName);
        if (locName.empty() || locName[0] != '/')
            locName.insert(0, "/");
        if (locName.length() > 1 && locName[locName.length() - 1] != '/')
            locName += '/';

        resolvedPath = locName + redirectTarget;

        size_t pos;
        while ((pos = resolvedPath.find("//")) != string::npos)
            resolvedPath.replace(pos, 2, "/");
    }

    return resolvedPath;
}

bool Response::detectRedirectLoop(const string& startPath, int maxDepth,
									set<string>& visitedPaths) {
    if (maxDepth <= 0 || visitedPaths.count(startPath)) return true;

    const location_b* loc = &findLocationConfig(startPath);
    if (!loc || loc->redirection.empty()) return false;

    string nextPath = resolveRedirect(loc->redirection, loc->name);

    visitedPaths.insert(startPath);
    return detectRedirectLoop(nextPath, maxDepth - 1, visitedPaths);
}


bool	Response::checkRedirections() {
	if (rules->redirection.empty() == false)
	{
		string currentPath = client->getRequest().getPath();
		set<string> visitedPaths;
		
        if (detectRedirectLoop(currentPath, 7, visitedPaths))
			return (this->client->setStatusCode(508), false);
		
		string resolvedPath = resolveRedirect(rules->redirection, rules->name);
		
		if (resolvedPath.empty())
			return (this->client->setStatusCode(500), false);
		
		this->setHeader("Location", resolvedPath);
		this->client->setStatusCode(301);
		return false;
	}
	return true;
}

void    Response::handleIndexFile(const string& requestMethod) {
    if (this->rules->index.empty()) return ;
    
    for (vector<string>::const_iterator it = this->rules->index.begin();
        it != this->rules->index.end(); ++it)
    {
        string indexPath = this->resourceInfo.path;
        if (!indexPath.empty() && indexPath[indexPath.length() - 1] != '/')
        indexPath += "/";
        
        indexPath += *it;
        
        file_info indexInfo;
        indexInfo.path = indexPath;
        indexInfo.type = getResourceType(indexInfo);
		indexInfo.pathDir = resourceInfo.pathDir;
        if (indexInfo.type == IS_FILE) {
            indexInfo.extension = getFileExtension(indexPath);
            checkcgiExtension(indexInfo);
            if (isCgiRequest || requestMethod != "POST") {
                resourceInfo = indexInfo;
                return;
            }
        }
        else if (indexInfo.type == IS_FORBIDDEN) {
            resourceInfo = indexInfo;
            return;
        }
    }
}


ResourceType	getResourceType(file_info& resource) {
	struct stat st;
	int ret = stat(resource.path.c_str(), &st);
	resource.size = (ret == 0) ? st.st_size : 0;

	if (ret != 0)
		return IS_NOT_FOUND;
	if (access(resource.path.c_str(), R_OK) != 0)
        return IS_FORBIDDEN;
	if (S_ISDIR(st.st_mode))
		return IS_DIR;
	if (S_ISREG(st.st_mode))
		return IS_FILE;
	return IS_NOT_FOUND;
}

void	Response::resolveResourcePath(const Request& request) {
	resourceInfo.pathDir = rules->path.empty() ?
		client->getServer()->root : rules->path;

	string normalizedLocName = rules->name;
	if (normalizedLocName.empty() || normalizedLocName[0] != '/')
		normalizedLocName.insert(0, "/");
	if (normalizedLocName.length() > 1 && normalizedLocName[normalizedLocName.length() - 1] != '/')
		normalizedLocName.push_back('/');

	string requestPath = request.getPath().length() > normalizedLocName.length() ?
		request.getPath().substr(normalizedLocName.length()) : "";

	if (!requestPath.empty() && requestPath[0] == '/')
		requestPath = requestPath.substr(1);
	
	resourceInfo.path = resourceInfo.pathDir;
	if (!resourceInfo.path.empty()
	&& resourceInfo.pathDir[resourceInfo.pathDir.length() - 1] != '/')
		resourceInfo.path += "/";

	resourceInfo.path += requestPath;
	resourceInfo.extension = getFileExtension(resourceInfo.path);
}

bool	Response::checkcgiExtension(file_info& resource) {
	isCgiRequest = false;
	for (vector<cgi_t>::const_iterator it = rules->cgiList.begin();
			it != rules->cgiList.end(); ++it)
	{
		if (it->extension == resource.extension)
		{	isCgiRequest = true; return true; }
	}
	return false;
}

int	Response::prepareResource(const Request& request) {
	resolveResourcePath(request);

	if (this->resourceInfo.path.find("..") != string::npos)
		return (client->setStatusCode(403), -1);

	resourceInfo.type = getResourceType(resourceInfo);
	
	if (resourceInfo.type == IS_DIR)
		handleIndexFile(request.getMethod());
	else if (resourceInfo.type == IS_FILE)
		checkcgiExtension(resourceInfo);

	if (resourceInfo.type == IS_NOT_FOUND || resourceInfo.type == IS_FORBIDDEN)
		return (client->setStatusCode(resourceInfo.type), -1);
	
	return (0);
}

int Response::prepareResponse() {
	Request& request = this->client->getRequest();
    this->rules = &(this->findLocationConfig(request.getPath()));

	if (checkRedirections() == false) return -1;

	if (find(rules->methods.begin(), rules->methods.end(),
			request.getMethod() ) == rules->methods.end() )
		return (this->client->setStatusCode(405), -1);

	if (prepareResource(request)) return -1;

	string sid = request.getCookie("session_id");
	if (!sid.empty())
		this->session = client->getGlobalConfig()->getSessionManager().getSession(sid);
	return (0);
}

string Response::serializeHeaders() {
    stringstream ss;

    ss << "HTTP/1.1 " << this->client->getStatusCode() << " " << getReasonPhrase() << CRLF;

    ss << "Server: Webserv/1.0" << CRLF;

    if (this->client->shouldKeepAlive()) {
        ss << "Connection: keep-alive" << CRLF;
        ss << "Keep-Alive: timeout=30" << CRLF;
    } else {
        ss << "Connection: close" << CRLF;
    }

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


string Response::getReasonPhrase() {
	switch (this->client->getStatusCode()) {
		case 200: return "OK";
		case 201: return "Created";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 413: return "Request Entity Too Large";
		case 414: return "Request-URI Too Long";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 505: return "HTTP Version Not Supported";
		case 504: return "Gateway Timeout";
		case 508: return "Loop Detected";
		default: return "Phrase";
	}
}

void Response::setErrorResponse(int code) {
	this->client->setStatusCode(code);
	this->rules = &(this->findLocationConfig(this->client->getRequest().getPath()));

	map<int, string>::const_iterator it = this->client->getServer()->errorPageMap.find(code);
	string content;
	if (it != this->client->getServer()->errorPageMap.end()) {
		ifstream file(it->second.c_str());
		if (file.is_open()) {
			content.assign((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
			file.close();
		}
	}
	if (content.empty()) {
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
		if (it->extension == this->resourceInfo.extension)
				return it->path;
	}
	return "";
}