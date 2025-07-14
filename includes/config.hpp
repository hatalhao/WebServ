#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "webserv.hpp"
#include "client.hpp"

class config {
    private:
        std::vector<Server> servers;
        std::vector<Client> clients;
};

#endif