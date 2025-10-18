#include "../../includes/response.hpp"
#include "../../includes/client.hpp"

Response::Response(Client* _client) :
        client(_client) , rules(NULL), session(NULL),
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
	struct stat statBuf;

	if (stat(resource, &statBuf) != 0)
		return (errno == ENOENT) ? IS_NOT_FOUND : IS_FORBIDDEN;
	else if (S_ISDIR(statBuf.st_mode)) return IS_DIR;
	else if (S_ISREG(statBuf.st_mode)) return IS_FILE;

	return IS_FORBIDDEN;
}

int Response::prepareResponse() {
    this->rules = &(this->findLocationConfig(this->client->getRequest().getPath()));
	// printSingleLocation(*this->rules);
	if (rules->redirection.empty() == false) {
	 	string currentPath = this->client->getRequest().getPath();

	 	std::set<string> visitedPaths;

        if (detectRedirectLoop(currentPath, 10, visitedPaths))
            return (this->client->setStatusCode(508), -1);

		string resolvedPath = resolveRedirect(rules->redirection, rules->name);

		if (resolvedPath.empty())
			return (this->client->setStatusCode(500), -1);

	 	this->setHeader("Location", resolvedPath);
		this->client->setStatusCode(301);
	 	return -1;
	 }

	if (find(rules->methods.begin(),
				rules->methods.end(),
				this->client->getRequest().getMethod()) == rules->methods.end())
		return (this->client->setStatusCode(405), -1);

	this->resourceDir = this->rules->path.empty() ? this->client->getServer()->root : this->rules->path;
	this->resourcePath = this->resourceDir + this->client->getRequest().getPath();

	if (!this->resourceDir.empty() && this->resourceDir[this->resourceDir.length() - 1] != '/'
		&& !this->rules->name.empty() && this->rules->name[0] != '/')
				this->resourceDir += "/";

	this->resourceDir += this->rules->name;

	size_t pos;
	while ((pos = this->resourcePath.find("//")) != string::npos)
		this->resourcePath.replace(pos, 2, "/");

	size_t dotPos = this->client->getRequest().getPath().find_last_of(".");
	if (dotPos != string::npos)
		this->resourceExtension = this->client->getRequest().getPath().substr(dotPos);
	else
		this->resourceExtension = "";

	this->resourceType = getResourceType(this->resourcePath.c_str());
	if (this->resourceType == IS_NOT_FOUND || this->resourceType == IS_FORBIDDEN)
		return (this->client->setStatusCode(this->resourceType), -1);

	if (this->resourceType == IS_DIR &&
		this->client->getRequest().getMethod() == "GET" &&
		!this->rules->index.empty()) {

		for (vector<string>::const_iterator it = this->rules->index.begin();
			 it != this->rules->index.end(); ++it) {
			string indexPath = this->resourcePath;
			if (!indexPath.empty() && indexPath[indexPath.length() - 1] != '/') {
				indexPath += "/";
			}
			indexPath += *it;

			ResourceType indexType = getResourceType(indexPath.c_str());
			if (indexType == IS_FILE) {
				this->resourcePath = indexPath;
				this->resourceType = IS_FILE;

				size_t dotPos = indexPath.find_last_of(".");
				if (dotPos != string::npos)
					this->resourceExtension = indexPath.substr(dotPos);
				else
					this->resourceExtension = "";
				break;
			}
		}
	}

	string	sid = this->client->getRequest().getCookie("session_id");
	if (!sid.empty())
		this->session = this->client->getGlobalConfig()->getSessionManager().getSession(sid);
	return (0);
}

string Response::serializeHeaders() {
    stringstream ss;

    ss << "HTTP/1.1 " << this->client->getStatusCode() << " " << getReasonPhrase() << CRLF;

    ss << "Server: Webserv/1.0" << CRLF;
    
    // Set Connection header based on keep-alive status
    if (this->client->shouldKeepAlive()) {
        ss << "Connection: keep-alive" << CRLF;
        ss << "Keep-Alive: timeout=5" << CRLF;  // 5 second timeout, max 100 requests
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


const string	Response::generatefilename() {
	stringstream ss;
	ss << "file_" << time(NULL) << ".upload";
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