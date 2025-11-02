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
	
	// Find the first boundary
	size_t startPos = bodyStr.find(fullBoundary);
	if (startPos == string::npos)
		return false;
	
	startPos += fullBoundary.length();
	
	// Skip CRLF after boundary
	if (startPos + 2 <= bodyStr.length() && bodyStr.substr(startPos, 2) == "\r\n")
		startPos += 2;
	
	// Find the end of headers (double CRLF)
	size_t headerEnd = bodyStr.find("\r\n\r\n", startPos);
	if (headerEnd == string::npos)
		headerEnd = bodyStr.find("\n\n", startPos);
	
	if (headerEnd == string::npos)
		return false;
	
	// Content starts after headers
	size_t contentStart = (bodyStr[headerEnd] == '\r') ? headerEnd + 4 : headerEnd + 2;
	
	// Find the ending boundary
	size_t endPos = bodyStr.find("\r\n" + fullBoundary, contentStart);
	if (endPos == string::npos)
		endPos = bodyStr.find("\n" + fullBoundary, contentStart);
	
	if (endPos == string::npos)
		return false;
	
	// Extract file content
	fileContent.assign(body.begin() + contentStart, body.begin() + endPos);
	return true;
}

static string extractBoundaryFromContentType(const string& contentType) {
	size_t boundaryPos = contentType.find("boundary=");
	if (boundaryPos == string::npos)
		return "";
	
	string boundary = contentType.substr(boundaryPos + 9);
	// Remove quotes if present
	if (!boundary.empty() && boundary[0] == '"')
		boundary = boundary.substr(1);
	if (!boundary.empty() && boundary[boundary.length() - 1] == '"')
		boundary = boundary.substr(0, boundary.length() - 1);
	
	return boundary;
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
		if (!file.is_open()) return 409;
		file.write(dataToWrite.data(), dataToWrite.size());
		file.close();

		setHeader("Location", headerlocationPath);
		setHeader("Content-Length", "0");
		return 201;
	}
	else
		return (setHeader("Allow", "GET"), setHeader("Content-Length", "0"), 405);
}
