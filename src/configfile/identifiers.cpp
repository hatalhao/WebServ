#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/tokenizer.hpp"

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
	return (key == "server_name" || key == "listen" || key == "index" || key == "allow" || key == "rewrite");
}