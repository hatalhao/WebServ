#pragma once

#include "origin.hpp"

typedef struct  Session
{
    map<string, string> data;
    time_t              last_access;
    
    Session() : last_access(time(0)) {}
} Session;


class SessionManager {
    private:
        map<string, Session>    sessions;
        string                  generateSID();
    
    public:
        string      createSession();
        Session*    getSession(const string& sid);
        void        deleteSession(const string& sid);
};