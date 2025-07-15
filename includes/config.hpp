#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "webserv.hpp"
#include "client.hpp"

class config {
    private:
        vector<Server> servers;
        vector<Client> clients;
};

#endif