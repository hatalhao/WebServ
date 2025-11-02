#include "../../includes/methods.hpp"
#include "../../includes/manipulators.hpp"
#include "../../includes/response.hpp"
#include "../../includes/cgi.hpp"

string getMimeType(const string& filePath) {
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == NPOS) {
        return "application/octet-stream";
    }
    
    string extension = filePath.substr(dotPos);

    for (size_t i = 0; i < extension.length(); ++i) {
        extension[i] = tolower(extension[i]);
    }
    
    if (extension == ".html" || extension == ".htm") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".js") return "application/javascript";
    if (extension == ".json") return "application/json";
    if (extension == ".png") return "image/png";
    if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    if (extension == ".gif") return "image/gif";
    if (extension == ".svg") return "image/svg+xml";
    if (extension == ".ico") return "image/x-icon";
    if (extension == ".txt") return "text/plain";
    if (extension == ".pdf") return "application/pdf";
    if (extension == ".xml") return "application/xml";
    if (extension == ".zip") return "application/zip";
    if (extension == ".mp4") return "video/mp4";
    if (extension == ".mp3") return "audio/mpeg";
    
    return "application/octet-stream";
}

bool isSafePath(const string& path) {
    return (path.find("..") == NPOS);
}

string generateDirectoryListing(const string& dirPath, const string& requestPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) {
        return "";
    }
    
    stringstream html;
    html << "<!DOCTYPE html>\n";
    html << "<html>\n<head>\n";
    html << "<title>Index of " << requestPath << "</title>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 40px; }\n";
    html << "h1 { color: #333; }\n";
    html << "table { border-collapse: collapse; width: 100%; }\n";
    html << "th, td { text-align: left; padding: 8px 12px; border-bottom: 1px solid #ddd; }\n";
    html << "th { background-color: #f5f5f5; }\n";
    html << "a { text-decoration: none; color: #0066cc; }\n";
    html << "a:hover { text-decoration: underline; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    html << "<h1>Index of " << requestPath << "</h1>\n";
    html << "<table>\n";
    html << "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";

    if (requestPath != "/") {
        string parentPath = requestPath;
        if (!parentPath.empty() && parentPath[parentPath.length() - 1] == '/') {
            parentPath = parentPath.substr(0, parentPath.length() - 1);
        }
        size_t lastSlash = parentPath.find_last_of('/');
        if (lastSlash != string::npos) {
            parentPath = parentPath.substr(0, lastSlash + 1);
        } else {
            parentPath = "/";
        }
        html << "<tr><td><a href=\"" << parentPath << "\">../</a></td><td>-</td><td>-</td></tr>\n";
    }
    
    vector<string> entries;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        string fileName = entry->d_name;
        if (fileName != "." && fileName != "..") {
            entries.push_back(fileName);
        }
    }
    closedir(dir);

    sort(entries.begin(), entries.end());

    for (vector<string>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        string fullPath = dirPath + "/" + *it;
        struct stat fileStat;
        
        string displayName = *it;
        string size = "-";
        string lastModified = "-";
        
        if (stat(fullPath.c_str(), &fileStat) == 0) {
            if (S_ISDIR(fileStat.st_mode)) {
                displayName += "/";
            } else {
                size = to_string(fileStat.st_size);
            }

            char timeStr[100];
            struct tm* timeInfo = localtime(&fileStat.st_mtime);
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", timeInfo);
            lastModified = timeStr;
        }
        
        string href = requestPath;
        if (!href.empty() && href[href.length() - 1] != '/') {
            href += "/";
        }
        href += *it;
        
        html << "<tr><td><a href=\"" << href << "\">" << displayName << "</a></td>";
        html << "<td>" << size << "</td>";
        html << "<td>" << lastModified << "</td></tr>\n";
    }
    
    html << "</table>\n";
    html << "</body>\n</html>";
    
    return html.str();
}

bool readFileContent(const string& filePath, string& content) {
    ifstream file(filePath.c_str(), ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);
    
    content.resize(fileSize);
    file.read(&content[0], fileSize);
    file.close();
    
    return true;
}

int Response::Get() {

    if (!isSafePath(this->client->getRequest().getPath())) {
        return (403);
    }

    switch (resourceInfo.type) {
        case IS_FILE: {

            if (getResourceType(this->resourceInfo) != IS_FILE) {
                return (this->setErrorResponse(403), 403);
            }

            int fd = open(resourceInfo.path.c_str(), O_RDONLY);
            if (fd < 0) {
                perror("open");
                return 500;
            }

            this->setHeader("Content-Type", getMimeType(resourceInfo.path));
            this->setHeader("Content-Length", to_string(resourceInfo.size));
            this->bodyFd = fd;
            this->bodySize = resourceInfo.size;
            this->bodySent = 0;
            this->isFileBody = true;

            return 200;
        }
        case IS_DIR: {
            if (!this->rules->index.empty()) {
                for (vector<string>::const_iterator it = this->rules->index.begin(); 
                    it != this->rules->index.end(); ++it) {
                    string indexPath = resourceInfo.path;
                    if (!indexPath.empty() && indexPath[indexPath.length() - 1] != '/') {
                        indexPath += "/";
                    }
                    indexPath += *it;
                    
                    struct stat indexStat;
                    if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode)) {
                        int fd = open(indexPath.c_str(), O_RDONLY);
                        if (fd < 0) {
                            perror("open");
                            return 403;
                        }
                        
                        this->setHeader("Content-Type", getMimeType(indexPath));
                        this->setHeader("Content-Length", to_string(indexStat.st_size));

                        // Use streaming approach for index files too
                        this->bodyFd = fd;              // keep fd for streaming
                        this->bodySize = indexStat.st_size;    // track remaining size
                        this->bodySent = 0;             // how much we already sent
                        this->isFileBody = true;

                        return 200;
                    }
                }
                return (this->setErrorResponse(404), 404);
            }
            else if (this->rules->isAutoIndexEnabled) {
                string listing = generateDirectoryListing(resourceInfo.path, this->client->getRequest().getPath());
                if (listing.empty()) {
                    return 500;
                }
                
                this->setHeader("Content-Type", "text/html");
                this->setHeader("Content-Length", to_string(listing.length()));
                this->setBody(listing);
                return 200;
            } else
                return 403;
        }
        default:
            return 403;
    }
}
