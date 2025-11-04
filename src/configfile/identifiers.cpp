#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/tokenizer.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/Config.hpp"

bool	isdefaultLocationFound(const server& serv) {
	for (size_t i = 0; i < serv.locations.size(); i++) {
		if (serv.locations[i].name == "/")
			return true;
	}
	return false;
}

bool notValidChar(const char c) {
	return (c == '\"' || c == '\'' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '[' || c == ']');
}

bool trimmableChar(const char c) {
	return (c == ' ' || c == '\t' || c == '[' || c == ']');
}

static inline bool	invalidBrackets(const std::string& line) {
	return (std::count(line.begin(), line.end(), '[') != 2 || \
		std::count(line.begin(), line.end(), ']') != 2 || line.find("[[") == NPOS \
		|| line.find("]]") == NPOS || line.find("[[") >= line.find("]]"));
}

static inline std::string extractDirective(const std::string& line) {
	std::string directive = line;
	size_t start = directive.find_first_not_of(" \t[");
	size_t end = directive.find_last_not_of(" \t]");
	if (start == NPOS || end == NPOS || end <= start)
		return ("invalid");
	directive = directive.substr(start, end - start + 1);
	return (directive);
}

int isValidBlock(const std::string& line) {
	if (line.find("=") != NPOS || invalidBrackets(line)) {
		return false;
	}

	std::string directive = extractDirective(line);
	std::string	directiveArr[] = {"invalid", "server", "location", "cgi", "error_pages"};
	size_t i = 4;
	for (; i > 0 && directiveArr[i] != directive; --i);
	return (i);
}

bool skipLine(const std::string& line) {
	size_t pos = line.find_first_not_of(" \t\n\r");
	return (pos == NPOS || line[pos] == '#');
}

bool	isKeyValue(const std::string& line, const int token) {
	std::string trimmedLine = line;
	removeInvalidChars(trimmedLine);
	size_t pos = trimmedLine.find('=');
	return (pos != NPOS && pos > 0 && pos < trimmedLine.length() - 1 && \
	std::count(trimmedLine.begin(), trimmedLine.end(), '=') == 1 && \
	isValidKey(trimmedLine.substr(0, pos - (trimmedLine[pos - 1] == ' ')), token));
}

void	minimumRequirement(std::string &line, std::ifstream& configFile) {
	while (std::getline(configFile, line)) {
		if (!skipLine(line))
			break ;
		}
	if (isValidBlock(line) != 1)
		throw std::runtime_error("Invalid configuration file");
}

bool	isArray(const std::string& line) {
	if (line.empty())
		return false;
	size_t pos = line.find_first_not_of(" \t\n\r");
	return (pos != NPOS && line[pos] == '[' && line.find(']') != NPOS);
}

bool	isArrayKey(const std::string& key) {
	return (key == "server_name" || key == "listen" || key == "index" \
		|| key == "allow");
}

bool isValidValue(const std::string& value) {
	string trimmedValue = value;
	removeInvalidChars(trimmedValue);
	if (trimmedValue.empty())
		return false;

	stringstream ss(trimmedValue);
	char firstChar = trimmedValue[0];
	char lastChar = trimmedValue[trimmedValue.length() - 1];
	if (firstChar == ']' || firstChar == '[' || lastChar == '[' || lastChar == ']')
		return false;
	string tmp;
	ss >> tmp;
	return (ss.eof() && !tmp.empty());
}

bool isValidMethod(const std::string& method) {
	std::string methods[] = {"GET", "POST", "DELETE"};
	for (size_t i = 0; i < 3 ; ++i)
		if (methods[i] == method) return true;
	return (false);
}

bool	isValidLocation(location_b& location, const std::string& root) {
	if (location.name.empty())
		return false;

	if (location.path.empty()) {
		location.path = root;
	}
	
	if (location.methods.empty())
		location.methods.push_back("GET");

	for (size_t i = 0; i < location.methods.size(); ++i)
		if (location.methods[i] == "POST" && location.uploadPath.empty()) { return false ;}
	return true;
}

int isValidStatusCode(const long statusCode) {
	if (statusCode < 400 || statusCode > 599)
		return -1;
	return static_cast<int>(statusCode);
}

bool isValidKey(const std::string& key, const int context) {
	long code = 0;

	switch (context) {
		case SERVER_TOKEN:
			if (key == "server_name" || key == "host" || key == "listen" || key == "root" || key == "index" || \
			key == "error_pages" || key == "location" || key == "cgi" || key == "max_body_size")
				return true;
			break;
		case LOCATION_TOKEN:
			if (key == "name" || key == "path" || key == "index" || key == "allow" || key == "autoindex" || \
			key == "upload_path" || key == "return")
				return true;
			break;
		case CGI_TOKEN:
			if (key == "extension" || key == "handler")
				return true;
			break;
		case ERROR_TOKEN:
			code = std::strtol(key.c_str(), 0, 10);
			if (code >= 400 && code <= 599)
				return true;
			break;
		default:
			return false;
	}
	return false;
}

bool	isValidHost(const std::string& host) {
	std::stringstream ss(host);
	int16_t octet;
	int count = 0;
	while (ss >> octet) {
		if (octet > 255 || octet < 0) {
			return false;
		}
		count++;
		if (ss.peek() == '.') {
			ss.ignore();
		} else if (ss.peek() != EOF) {
			return false;
		}
	}
	return (count == 4 && ss.eof());
}