 #include "../../includes/response.hpp"
#include "../../includes/statuscode.hpp"
#include "../../includes/methods.hpp"

bool DeletePermitted(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    std::string parentDir = (lastSlash != NPOS) ? path.substr(0, lastSlash) : ".";
    
    if (parentDir.empty()) {
        parentDir = "/";
    }
    
    return (access(parentDir.c_str(), W_OK) == 0);
}

bool deleteResource(const std::string& path) {
    struct stat statBuf;
    
    if (stat(path.c_str(), &statBuf) != 0) { return false; }
    
    if (S_ISREG(statBuf.st_mode)) {
        if (!DeletePermitted(path))
            return false;
        return (unlink(path.c_str()) == 0);
    }
    
    if (S_ISDIR(statBuf.st_mode)) {
        if (!DeletePermitted(path)) { return false; }
        
        DIR* dir = opendir(path.c_str());
        if (!dir) { return false;}
        
         dirent* entry;
        bool success = true;
        
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string entryPath = path + "/" + entry->d_name;

            if (!deleteResource(entryPath)) {
                success = false;
                break;
            }
        }
        
        closedir(dir);
        
        if (success) {
            success = (rmdir(path.c_str()) == 0);
        }

        return success;
    }
    
    // REMINDER: This check is for symbolic links or other types
    if (!DeletePermitted(path)) {
        return false;
    }
    return (unlink(path.c_str()) == 0);
}

int Response::Delete() {
    struct stat statBuf;
    
	if (stat(this->resourceInfo.path.c_str(), &statBuf) != 0) {
        if (errno == ENOENT) {
            return NOT_FOUND;
        } else {
            return FORBIDDEN;
        }
    }
    
    if (!DeletePermitted(this->resourceInfo.path)) {
        return FORBIDDEN;
    }
    
    if (deleteResource(this->resourceInfo.path)) {
        return 200;
    } else {
        if (errno == EACCES || errno == EPERM) {
            return FORBIDDEN;
        } else if (errno == ENOENT) {
            return NOT_FOUND;
        } else {
            return INTERNAL_SERVER_ERROR;
        }
    }
}
