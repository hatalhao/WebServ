// #include "../../includes/origin.hpp"

// std::string createErrorResponse(int statusCode, const std::string& message) {
// 	std::stringstream response;
// 	response << "HTTP/1.1 " << statusCode << " " << message << "\r\n";
// 	response << "Content-Type: text/plain\r\n";
// 	response << "Content-Length: " << message.length() << "\r\n";
// 	response << "\r\n";
// 	response << message;
// 	return response.str();
// }

// std::string getContentType(const std::string& path) {
// 	fstream file();
// }

// std::string handleGetRequest(const HttpRequest& request, const ServerConfig& config) {
// 	std::string path = request.getPath();
	
// 	// Handle root path
// 	if (path == "/") {
// 		path = config.getDocumentRoot() + "/index.html";
// 	} else {
// 		path = config.getDocumentRoot() + path;
// 	}
	
// 	// Check if file exists and is readable
// 	std::ifstream file(path.c_str());
// 	if (!file.is_open()) {
// 		return createErrorResponse(404, "Not Found");
// 	}
	
// 	// Read file content
// 	std::stringstream buffer;
// 	buffer << file.rdbuf();
// 	std::string content = buffer.str();
// 	file.close();
	
// 	// Determine content type
// 	std::string contentType = getContentType(path);
	
// 	// Build HTTP response
// 	std::stringstream response;
// 	response << "HTTP/1.1 200 OK\r\n";
// 	response << "Content-Type: " << contentType << "\r\n";
// 	response << "Content-Length: " << content.length() << "\r\n";
// 	response << "\r\n";
// 	response << content;
	
// 	return response.str();
// }