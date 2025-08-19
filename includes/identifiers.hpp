#pragma once

#include "origin.hpp"

void	minimumRequirement(std::string &line, std::ifstream& configFile);
int		isValidBlock(const std::string& line);
bool	isValidLocation(location_b& location, const std::string& root);
bool	notValidChar(char c);
bool	trimmableChar(char c);
bool	skipLine(const std::string& line);
bool	isKeyValue(const std::string& line);
bool	isArray(const std::string& line);
bool	isArrayKey(const std::string& key);
bool 	isValidMethod(const std::string& method);
int		isValidStatusCode(long statusCode);