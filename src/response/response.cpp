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
// 	cout << "Location Block:" << endl;
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

const location_b&	Response::findLocationConfig() {
	const location_b *best = NULL, *root = NULL;
	size_t	bestLen = 0;	
	for (vector<location_b>::iterator it = this->client->getServer()->locations.begin();
		it != this->client->getServer()->locations.end(); it++)
	{
		if (it->name == "/")
			root = &(*it);
		if (it->name[0] != '/') it->name.insert(0, "/");
		if (this->client->getRequest().getPath().find(it->name) == 0) {
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
	if (find(rules->methods.begin(),
				rules->methods.end(),
				this->client->getRequest().getMethod()) == rules->methods.end())
		return (this->client->setStatusCode(405), -1);

	this->resourceDir = this->rules->path.empty() ? this->client->getServer()->root : this->rules->path;
	this->resourcePath = this->resourceDir + this->client->getRequest().getPath();

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

	// If resource is a directory and we're doing a GET request, check for index files
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
				// Found a valid index file, update resource path and type
				this->resourcePath = indexPath;
				this->resourceType = IS_FILE;
				
				// Update resource extension
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
	switch (this->client->getStatusCode()) {
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
	this->client->setStatusCode(code);
	this->rules = &(this->findLocationConfig());

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