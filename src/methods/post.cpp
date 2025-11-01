#include "../../includes/response.hpp"
#include "../../includes/cgi.hpp"


const string	generateRandomFileName() {
	stringstream ss;
	ss <<  "file_" << time(NULL) << ".upload";
	return ss.str();
}

int	Response::Post() {
	if (this->resourceType != IS_FILE) {
		Request	&request = client->getRequest();
		if (getResourceType(resourceDir.c_str()) != IS_DIR) return (500);

		string filename = this->resourcePath;
		string headerlocationPath = request.getPath();

		if (this->resourceType == IS_DIR) {
			if (filename[filename.length() - 1] != '/')
			{ filename += "/";  headerlocationPath += "/";  }
			string randomName = generateRandomFileName();
			filename += randomName;
			headerlocationPath += randomName;
		}
		ofstream file(filename.c_str(), ios::binary);
		if (!file.is_open()) return 409;
		file.write(request.getBody().data(), request.getBody().size());
		file.close();

		setHeader("Location", headerlocationPath);
		setHeader("Content-Length", "0");
		return 201;
	}
	else
		return (setHeader("Allow", "GET"), setHeader("Content-Length", "0"), 405);
}
