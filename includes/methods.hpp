#pragma once

#include "origin.hpp"
#include "response.hpp"
#include "request.hpp"
#include <dirent.h>

int handleGet(const Request& request, Response& response, const location_b& rules, 
              const string& resourcePath, ResourceType resourceType);

string getMimeType(const string& filePath);
string toString(size_t value);
string toString(int value);
bool isSafePath(const string& path);
string generateDirectoryListing(const string& dirPath, const string& requestPath);
bool readFileContent(const string& filePath, string& content);
