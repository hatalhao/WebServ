#include "../../includes/cgi.hpp"

CgiHandler::CgiHandler(const Response& _response, const string& _scriptPath) :
    Response(_response), scriptPath(_scriptPath) {}

void    CgiHandler::setupEnv() {
    env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env_vars.push_back("SERVER_SOFTWARE=WebServ/1.0");
    env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");

    env_vars.push_back("REQUEST_METHOD=" + this->request.getMethod());
    env_vars.push_back("QUERY_STRING=" + this->request.getQueryString());
    env_vars.push_back("SCRIPT_");

}

void    CgiHandler::execute() {

}