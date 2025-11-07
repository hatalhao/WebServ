#include "../../includes/response.hpp"
#include "../../includes/cgi.hpp"


const string	generateRandomFileName() {
	stringstream ss;
	ss <<  "file_" << time(NULL) << ".upload";
	return ss.str();
}

static bool extractMultipartFile(const vector<char>& body, const string& boundary, vector<char>& fileContent) {
	if (body.empty() || boundary.empty())
		return false;

	string fullBoundary = "--" + boundary;
	string bodyStr(body.begin(), body.end());
	
	size_t startPos = bodyStr.find(fullBoundary);
	if (startPos == string::npos)
		return false;
	
	startPos += fullBoundary.length();
	
	if (startPos + 2 <= bodyStr.length() && bodyStr.substr(startPos, 2) == "\r\n")
		startPos += 2;
	
	size_t headerEnd = bodyStr.find("\r\n\r\n", startPos);
	if (headerEnd == string::npos)
		headerEnd = bodyStr.find("\n\n", startPos);
	
	if (headerEnd == string::npos)
		return false;
	
	size_t contentStart = (bodyStr[headerEnd] == '\r') ? headerEnd + 4 : headerEnd + 2;

	size_t endPos = bodyStr.find("\r\n" + fullBoundary, contentStart);
	if (endPos == string::npos)
		endPos = bodyStr.find("\n" + fullBoundary, contentStart);
	
	if (endPos == string::npos)
		return false;
	
	fileContent.assign(body.begin() + contentStart, body.begin() + endPos);
	return true;
}

static string extractBoundaryFromContentType(const string& contentType) {
	size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos == string::npos)
		return "";
	
	string boundary = contentType.substr(boundaryPos + 9);
	if (!boundary.empty() && boundary[0] == '"')
		boundary = boundary.substr(1);
	if (!boundary.empty() && boundary[boundary.length() - 1] == '"')
		boundary = boundary.substr(0, boundary.length() - 1);
	
	return boundary;
}

void	Response::Post() {
	if (this->resourceInfo.type != IS_FILE) {
		Request	&request = client->getRequest();

		string filename = this->resourceInfo.path;
		string headerlocationPath = request.getPath();

		if (this->resourceInfo.type == IS_DIR) {
			if (filename[filename.length() - 1] != '/')

			{ filename += "/";  headerlocationPath += "/";  }
			string randomName = generateRandomFileName();
			filename += randomName;
			headerlocationPath += randomName;
		}
		
		string contentType = request.getHeader("content-type");
		vector<char> dataToWrite = request.getBody();
		
		if (contentType.find("multipart/form-data") != string::npos) {
			string boundary = extractBoundaryFromContentType(contentType);
			if (!boundary.empty()) {
				vector<char> fileContent;
				if (extractMultipartFile(request.getBody(), boundary, fileContent)) {
					dataToWrite = fileContent;
				}
			}
		}
		
		ofstream file(filename.c_str(), ios::binary);
		if (!file.is_open()) return this->setErrorResponse(409);
		file.write(dataToWrite.data(), dataToWrite.size());
		file.close();

		setHeader("Location", headerlocationPath);
		setHeader("Content-Length", "0");
		return this->client->setStatusCode(201);
	}
	else
		return (setHeader("Allow", "GET"), this->setErrorResponse(405));
}
