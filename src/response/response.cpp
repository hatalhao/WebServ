#include "../../includes/response.hpp"
#include "../../includes/client.hpp"

Response::Response(Client* _client) :
        client(_client) , rules(NULL), session(NULL), isCgiRequest(false),
		bodyFd(-1), bodySize(0), bodySent(0), isFileBody(false) {}

Response::Response(const Response& other) :
	client(other.client), rules(other.rules),
	resourcePath(other.resourcePath), resourceDir(other.resourceDir),
	resourceType(other.resourceType), headers(other.headers), body(other.body){}

// void printSingleLocation(const location_b& loc) {
// 	cout << "--Location Block:" << endl;
// 	cout << "  name: " << loc.name << endl;
// 	cout << "  path: " << loc.path << endl;
// 	cout << "  index: ";
// 	for (size_t i = 0; i < loc.index.size(); i++) {
// 		cout << loc.index[i];
// 		if (i < loc.index.size() - 1)
// 			cout << ", ";
// 	}
// 	cout << endl;
// 	cout << "  errorPageMap: ";
// 	for (map<int, string>::const_iterator it = loc.errorPageMap.begin(); it !=
// 			loc.errorPageMap.end(); ++it) {
// 		cout << it->first << " -> " << it->second << "; ";
// 	}
// 	cout << endl;
// 	cout << "  methods: ";
// 	for (size_t i = 0; i < loc.methods.size(); i++) {
// 		cout << loc.methods[i];
// 		if (i < loc.methods.size() - 1)
// 			cout << ", ";
// 	}
// 	cout << endl;
// 	cout << "  redirection: " << loc.redirection << endl;
// 	cout << "  uploadPath: " << loc.uploadPath << endl;
// 	cout << "  isAutoIndexEnabled: " << (loc.isAutoIndexEnabled ? "true" : "false") << endl;
// 	cout << "  isCgiEnabled: " << (loc.isCgiEnabled ? "true" : "false") << endl;
// 	cout << "  cgiList: ";
// 	for (size_t i = 0; i < loc.cgiList.size(); i++) {
// 		cout << "{" << loc.cgiList[i].extension << " -> " << loc.cgiList[i].path << "}";
// 		if (i < loc.cgiList.size() - 1)
// 			cout << ", ";
// 	}
// 	cout << endl;

// }

const string	getFileExtension(const string& filename) {
	size_t dotPos = filename.find_last_of(".");
	if (dotPos != string::npos)
		return filename.substr(dotPos);
	return "";
}

const location_b& Response::findLocationConfig(const string& path) {
    const location_b* bestMatch = NULL, *root = NULL;
    size_t longestMatchLen = 0;
    string normalizedPath(path);

    if (normalizedPath.empty() || normalizedPath[0] != '/')
        normalizedPath.insert(0, "/");

    string pathWithSlash = normalizedPath;
    if (pathWithSlash[pathWithSlash.length() - 1] != '/')
        pathWithSlash += '/';

    for (vector<location_b>::const_iterator it = this->client->getServer()->locations.begin();
         it != this->client->getServer()->locations.end(); ++it) {
		if (it->name == "/") root = &(*it);

        string normalizedLocName = it->name;
        if (normalizedLocName.empty() || normalizedLocName[0] != '/')
            normalizedLocName.insert(0, "/");
        if (normalizedLocName.length() > 1 && normalizedLocName[normalizedLocName.length() - 1] != '/')
            normalizedLocName.push_back('/');

        bool matches = (normalizedPath.rfind(normalizedLocName, 0) == 0) ||
                      (pathWithSlash.rfind(normalizedLocName, 0) == 0);

        if (matches) {
            if (normalizedLocName.length() > longestMatchLen) {
                longestMatchLen = normalizedLocName.length();
                bestMatch = &(*it);
            }
        }
    }

    if (!bestMatch)
        bestMatch = root;
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


ResourceType	getResourceType(const char* resource) {
	struct stat st;
	if (stat(resource, &st) != 0)
		return IS_NOT_FOUND;

	if (access(resource, R_OK) != 0)
        return IS_FORBIDDEN;

    if (S_ISDIR(st.st_mode))
        return IS_DIR;
    if (S_ISREG(st.st_mode))
        return IS_FILE;
    return IS_NOT_FOUND;
}


void	Response::resolveResourcePath(bool isPOST) {
	if (this->isCgiRequest || !isPOST)
		this->resourceDir = rules->path.empty() ?
				client->getServer()->root : rules->path;
	else
		this->resourceDir = rules->uploadPath;

	string requestPath = client->getRequest().getPath().length() > rules->name.length() ?
		client->getRequest().getPath().substr(rules->name.length()) : "";

	if (!requestPath.empty() && requestPath[0] == '/')
		requestPath = requestPath.substr(1);
	
	this->resourcePath = resourceDir;
	if (!this->resourcePath.empty() && resourceDir[resourceDir.length() - 1] != '/')
		this->resourcePath += "/";

	this->resourcePath += requestPath;

	this->resourceExtension = getFileExtension(requestPath);	
}

bool	Response::checkRedirections() {
	if (rules->redirection.empty() == false) {
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


void	Response::handleIndexFile() {
	if (this->resourceType != IS_DIR || this->rules->index.empty())
		return ;

	for (vector<string>::const_iterator it = this->rules->index.begin();
			it != this->rules->index.end(); ++it)
	{
		string indexPath = this->resourcePath;
		if (!indexPath.empty() && indexPath[indexPath.length() - 1] != '/')
			indexPath += "/";

		indexPath += *it;

		if (getResourceType(indexPath.c_str()) == IS_FILE) {
			this->resourcePath = indexPath;
			this->resourceType = IS_FILE;
			this->resourceExtension = getFileExtension(indexPath);
			break;
		}
	}
}

int Response::prepareResponse() {
	Request& request = this->client->getRequest();
    this->rules = &(this->findLocationConfig(request.getPath()));

	// printSingleLocation(*this->rules);
	if (checkRedirections() == false) return -1;

	if (find(rules->methods.begin(), rules->methods.end(),
			request.getMethod() ) == rules->methods.end() )
		return (this->client->setStatusCode(405), -1);

	this->resourceExtension = getFileExtension(request.getPath());
	this->isCgiRequest = false;
	for (vector<cgi_t>::const_iterator it = rules->cgiList.begin(); it != rules->cgiList.end(); ++it) {

		if (it->extension == this->resourceExtension) {
			this->isCgiRequest = true;
			break;
		}
	}

	const string& requestMethod = request.getMethod();

	this->resolveResourcePath(requestMethod == "POST");
	if (this->resourcePath.find("..") != string::npos)
		return (this->client->setStatusCode(403), -1);

	this->resourceType = getResourceType(this->resourcePath.c_str());

	if (this->resourceType == IS_FORBIDDEN
		|| (this->resourceType == IS_NOT_FOUND && requestMethod != "POST") )
		return (this->client->setStatusCode(this->resourceType), -1);

	// if (requestMethod == "GET") handleIndexFile();

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
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 413: return "Request Entity Too Large";
		case 414: return "Request-URI Too Long";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 505: return "HTTP Version Not Supported";
		case 508: return "Loop Detected";
		default: return "OK";
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
		if (it->extension == this->resourceExtension)
				return it->path;
	}
	return "";
}