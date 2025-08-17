#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "request.hpp"

class Response {
    private:
        const vector<location_b>& routes;
        const Request&            request;
        const location_b&	findLocationConfig();

    public:
        Response(const vector<location_b>& locations, const Request& request);
        int checkRequirements();
};

#endif