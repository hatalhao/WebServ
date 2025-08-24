#include "../../includes/cgi.hpp"

CgiHandler::CgiHandler(const Response& _response, const string& _scriptPath) :
    Response(_response), scriptPath(_scriptPath) {}

void    CgiHandler::setupEnv() {
    env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env_vars.push_back("SERVER_SOFTWARE=WebServ/1.0");
    env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");

    // env_vars.push_back("SERVER_NAME=" + this->serverConf->serverName);
    // env_vars.push_back("SERVER_PORT=" + this->currentPort);
    env_vars.push_back("REQUEST_METHOD=" + this->request.getMethod());
    env_vars.push_back("QUERY_STRING=" + this->request.getQueryString());
    env_vars.push_back("SCRIPT_NAME=" + this->request.getPath());

    env_vars.push_back("PATH_INFO=" + this->scriptPath);

    if (this->request.getMethod() == "POST") {
        env_vars.push_back("CONTENT_TYPE=" + this->request.getHeader("content-type"));
        if (this->request.getHeader("content-length") != "")
            env_vars.push_back("CONTENT_LENGTH=" + this->request.getHeader("content-length"));
        else {
            env_vars.push_back("CONTENT_LENGTH=" + to_string(this->request.getBody().size()));
        }
    }
}

void    CgiHandler::execute() {

}