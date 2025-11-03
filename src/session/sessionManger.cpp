#include "../../includes/Session.hpp"

string  SessionManager::generateSID() {
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
    if (!urandom) {
        // Fallback to std::rand() if urandom is not available
        std::string fallback_sid;
        for (std::size_t i = 0; i < 32; ++i)
            fallback_sid += charset[std::rand() % (sizeof(charset) - 1)];
        return fallback_sid;
    }

    std::string sid;
    for (std::size_t i = 0; i < 32; ++i) {
        char byte;
        urandom.read(&byte, 1);
        unsigned char index = static_cast<unsigned char>(byte) % (sizeof(charset) - 1);
        sid += charset[index];
    }

    urandom.close();
    return sid;
}

string  SessionManager::createSession() {
    string  sessionID = generateSID();
    this->sessions[sessionID] = Session();
    return sessionID;
}

Session*    SessionManager::getSession(const string& sid) {
    if (this->sessions.find(sid) != this->sessions.end()) {
        this->sessions[sid].last_access = time(0);
        return &this->sessions[sid];
    }
    return NULL;
}

void    SessionManager::deleteSession(const string& sid) { sessions.erase(sid); }
