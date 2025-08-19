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

	this->resourceType = getResourceType(this->resourcePath.c_str());
	return (0);
}


const string	Response::generatefilename(const string& basePath) {
	stringstream ss;
	ss << basePath << "/file_" << time(NULL) << ".upload";
	return ss.str();
}

int	Response::Post() {
	if (this->resourceType == IS_DIR) {
		// if (!this->rules->uploadPath.empty()) 
		// {
		// 	if (getResourceType(this->rules->uploadPath.c_str()) != IS_DIR) return (403);
		// 	std::ofstream outfile(generatefilename(this->rules->uploadPath).c_str());
		// 	if (!outfile.is_open())
		// 		return 403;
		// 	outfile.write(this->request.getBody().data(), this->request.getBody().size());
		// 	outfile.close();
		// }
		// else
		// 	return 500;
	}
	else if (this->resourceType == IS_FILE) {
		
	}
	return (0);
}
