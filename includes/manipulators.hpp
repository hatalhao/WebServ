#pragma once

#include "origin.hpp"

template<typename T>
const std::string to_string(const T& value) {
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

size_t		parseSize(const std::string& sizeStr);
void		removeInvalidChars(std::string &str);
std::string	saveArray(const std::string& line);