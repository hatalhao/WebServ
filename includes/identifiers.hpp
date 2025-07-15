#pragma once

#include "origin.hpp"

bool notValidChar(char c);
bool trimmableChar(char c);
bool isValidBlock(const std::string& line);
bool skipLine(const std::string& line);
bool isKeyValue(const std::string& line);
void minimumRequirement(std::string &line, std::ifstream& configFile);
bool isArray(const std::string& line);