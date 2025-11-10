#include "../../includes/response.hpp"
#include "../../includes/cgi.hpp"


const string	generateRandomFileName() {
	static unsigned int counter = 0;
	stringstream ss;
	ss <<  "file_" << time(NULL) << "_" << getpid() << "_" << (++counter) << ".upload";
	return ss.str();
}

bool	Response::preparePostDestination() {
	if (resourceInfo.type != IS_DIR || rules->uploadPath.empty()) {
		setHeader("Allow", "GET");
		client->setStatusCode(405);
		return false;
	}

	Request	&request = client->getRequest();

	string filename = rules->uploadPath;
	string headerlocationPath = request.getPath();

	if (!filename.empty() && filename[filename.length() - 1] != '/')
		filename += "/";
	if (!headerlocationPath.empty() && headerlocationPath[headerlocationPath.length() - 1] != '/')
		headerlocationPath += "/";
	
	string randomName = generateRandomFileName();
	filename += randomName;
	headerlocationPath += randomName;
	
	this->uploadFilePath = filename;
	this->uploadLocationPath = headerlocationPath;
	
	int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return (client->setStatusCode(409), false);
	
	bodyFd = fd;
	return true;
}

void	Response::finalizePost() {
	if (bodyFd >= 0) {
		close(bodyFd);
		bodyFd = -1;
	}
	
	setHeader("Location", uploadLocationPath);
	setHeader("Content-Length", "0");
	client->setStatusCode(201);
}

void	Response::Post() {
	if (!preparePostDestination()) {
		setErrorResponse(client->getStatusCode());
		return;
	}
}
