#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/tokenizer.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/Config.hpp"

std::string ServerNameExists(const std::string& serverName, Config& config) {
	if (serverName.find_first_not_of("1234567890.") == NPOS)
		return (serverName);
	for (size_t i = 0; i < config._servers.size(); ++i) {
		for (size_t j = 0; j < config._servers[i].serverName.size(); ++j) {
			if (config._servers[i].serverName[j] == serverName)
				return (config._servers[i].host);
		}
	}
	return "";
}

bool notValidChar(char c) {
	return (c == '\"' || c == '\'' || c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '[' || c == ']');
}

bool trimmableChar(char c) {
	return (c == ' ' || c == '\t' || c == '[' || c == ']');
}

int isValidBlock(const std::string& line) {
	if (line.find("=") != NPOS || std::count(line.begin(), line.end(), '[') != 2 || \
	std::count(line.begin(), line.end(), ']') != 2 || line.find("[[") == NPOS \
	|| line.find("]]") == NPOS || line.find("[[") >= line.find("]]")) {
		return false;
	}
	std::string directive = line;
	directive.erase(std::remove_if(directive.begin(), directive.end(), trimmableChar), directive.end());
	std::string	directiveArr[] = {"invalid", "server", "location", "cgi", "error_pages"};
	size_t i = 4;
	for (; i > 0 && directiveArr[i] != directive; --i);
	return (i);
}

bool skipLine(const std::string& line) {
	size_t pos = line.find_first_not_of(" \t\n\r");
	return (pos == NPOS || line[pos] == '#');
}

bool	isKeyValue(const std::string& line) {
	std::string trimmedLine = line;
	removeInvalidChars(trimmedLine);
	size_t pos = trimmedLine.find('=');
	return (pos != NPOS && pos > 0 && pos < trimmedLine.length() - 1 && std::count(trimmedLine.begin(), trimmedLine.end(), '=') == 1);
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
		location.path = root + location.name;
	}
	
	if (location.methods.empty())
		location.methods.push_back("GET");

	for (size_t i = 0; i < location.methods.size(); ++i)
		if (location.methods[i] == "POST" && location.uploadPath.empty()) { return false ;}
	return true;
}

int isValidStatusCode(long statusCode) {
	if (statusCode != BAD_REQUEST && statusCode != FORBIDDEN && statusCode != NOT_FOUND && \
		statusCode != REQUEST_TIMEOUT && statusCode != REQUEST_ENTITY_TOO_LARGE && \
		statusCode != REQUEST_URI_TOO_LONG && statusCode != INTERNAL_SERVER_ERROR && \
		statusCode != NOT_IMPLEMENTED)
		return -1;
	return static_cast<int>(statusCode);
}