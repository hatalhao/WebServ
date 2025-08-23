#include "../../includes/response.hpp"

Response::Response(const server* _serverConf, const Request& _request) :
        serverConf(_serverConf) , request(_request), rules(NULL) {}

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
		this->rules = other.rules;
		this->resourcePath = other.resourcePath;
		this->resourceType = other.resourceType;
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
		return (405);

	string root = this->rules->path.empty() ? this->serverConf->root : this->rules->path;
	this->resourcePath = root + this->request.getPath();

	size_t pos;
	while ((pos = this->resourcePath.find("//")) != string::npos)
		this->resourcePath.replace(pos, 2, "/");

	struct stat statBuf;
	
	if (stat(this->resourcePath.c_str(), &statBuf) != 0)
		this->resourceType = (errno == ENOENT) ? IS_NOT_FOUND : IS_FORBIDDEN;
	else if (S_ISDIR(statBuf.st_mode)) this->resourceType = IS_DIR;
	else if (S_ISREG(statBuf.st_mode)) this->resourceType = IS_FILE;
	else this->resourceType = IS_FORBIDDEN;

	this->resourceType = getResourceType(this->resourcePath.c_str());
	return (0);
}

// Determine the Full File Path: Construct the absolute path to the resource on the server's disk.
//		You do this by combining the root directive from the matched location with the request's URI.

// Check the Resource (stat): Use the stat() system call on the full path to see what it is.

// If it's a file: Check if it's a CGI script based on its extension. If yes, execute the CGI handler.
//	If no, proceed to serve the static file (for GET), delete it (for DELETE), or handle an upload (for POST).

// If it's a directory:

// Check for an index file (e.g., index.html). If one exists, treat the request as a request for that file
// 					and restart this logic.

// If no index file is found, check if autoindex is on. If so, generate a directory listing page.
const string	Response::generatefilename() {
	stringstream ss;
	ss << "file_" << time(NULL) << ".upload";
	return ss.str();
}

// If autoindex is off, the action is to generate a 403 Forbidden error.

// If it doesn't exist: The action is to generate a 404 Not Found error.
void	Response::setHeader(string key, string value)
	{ this->headers.insert(pair<string, string>(key, value)); }

// Generate the Response: Based on the outcome of the step above
// (e.g., you have file content to send, a CGI script's output, or an error code),
// you will use your Response class to build the final HTTP response.
vector<cgi_t>::const_iterator	Response::searchExtensionHandler(const string& ext) {
	vector<cgi_t>::const_iterator it = this->rules->cgiList.begin();
	for (; it != this->rules->cgiList.end(); it++) {
		if (it->extension == ext)
				return it;
	}
	return it;
}

int	Response::Post() {
	if (this->resourceType == IS_DIR) {
		// if (!this->rules->uploadPath.empty())
		// {
		// 	if (getResourceType(this->rules->uploadPath.c_str()) != IS_DIR) return (500);
		// 	string	filename = generatefilename();
		// 	std::ofstream outfile((this->rules->uploadPath
		// 		+ (rules->uploadPath[rules->uploadPath.length() - 1] == '/' ? "" : "/")
		// 		+ filename).c_str(), ios::binary);
		// 	if (!outfile.is_open()) return 500;
		// 	outfile.write(this->request.getBody().data(), this->request.getBody().size());
		// 	outfile.close();
		// 	setHeader("Location", this->request.getPath()
		// 			+ (this->request.getPath()[request.getPath().length() - 1] == '/' ? "" : "/")
		// 			+ filename);
		// 	return 201;
		// }
		// else
		// 	return 500;
	}
	else if (this->resourceType == IS_FILE) {
		if (this->rules->cgiList.empty() == false) {
			if (searchExtensionHandler(this->request.getPath().substr(request.getPath().find_last_of(".")))
							!= this->rules->cgiList.end()) {
				// CGI WORK STARTS HERE
				return 200;
			}
		}
		else 
			return (setHeader("Allow", "GET"), 405);
	}
	else if (this->resourceType == IS_NOT_FOUND)
		return 404;
	return 403;
}
