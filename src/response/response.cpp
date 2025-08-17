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

	return (0);
}

const string	Response::generatefilename(string& basePath) {

}

int	Response::Post() {
	if (this->resourceType == IS_DIR) {
		if (this->rules->cgiList.empty()) {
			
		}
	}
	else if (this->resourceType == IS_FILE) {
		
	}
}


