#include "../includes/Session.hpp"

string  SessionManager::generateSID() {
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    ifstream urandom("/dev/urandom", ios::in | ios::binary);
    if (!urandom) {
        string fallback_sid;
        for (size_t i = 0; i < 32; ++i)
            fallback_sid += charset[rand() % (sizeof(charset) - 1)];
        return fallback_sid;
    }

    string sid;
    for (size_t i = 0; i < 32; ++i) {
        char byte;
        urandom.read(&byte, 1);
        if (!urandom || urandom.gcount() != 1) {
            urandom.close();
            string fallback_sid;
            for (size_t j = sid.length(); j < 32; ++j)
                fallback_sid += charset[rand() % (sizeof(charset) - 1)];
            return sid + fallback_sid;
        }
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
