#include "../../includes/response.hpp"
#include "../../includes/cgi.hpp"


int	Response::Post() {
	if (this->resourceType == IS_DIR) {
		if (!this->rules->uploadPath.empty())
		{
			if (getResourceType(this->rules->uploadPath.c_str()) != IS_DIR) return (500);
			string	filename = generatefilename();
			std::ofstream outfile((this->rules->uploadPath
				+ (rules->uploadPath[rules->uploadPath.length() - 1] == '/' ? "" : "/")
				+ filename).c_str(), ios::binary);
			if (!outfile.is_open()) return 500;
			outfile.write(this->request.getBody().data(), this->request.getBody().size());
			outfile.close();
			setHeader("Location", this->request.getPath()
					+ (this->request.getPath()[request.getPath().length() - 1] == '/' ? "" : "/")
					+ filename);
			setHeader("Content-Length", "0");
			return 201;
		}
		else
			return 500;
	}
	return (setHeader("Allow", "GET"), setHeader("Content-Length", "0"), 405);
}

string Response::serializeHeaders() {
    stringstream ss;

    ss << "HTTP/1.1 " << statusCode << " " << getReasonPhrase() << CRLF;

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
