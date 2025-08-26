#include "../../includes/cgi.hpp"

CgiHandler::CgiHandler(const Response& _response, const string& _scriptPath) :
	Response(_response), scriptPath(_scriptPath) {}

string	toLower(string& src) {
	string _new = src;
	for (size_t i = 0; i < _new.length(); i++)
		_new[i] = tolower(_new[i]);
	return (_new);
}

void	CgiHandler::populateResponse(const string& output) {
	size_t crlfPos = output.find(CRLF CRLF);
	string	_headers, _body, line;

	if (crlfPos != string::npos) {
		_headers = output.substr(0, crlfPos);
		_body = output.substr(crlfPos + CRLF_LENGTH + CRLF_LENGTH); 
	}
	else 
		_body = output;

	this->setBody(_body);

	stringstream ss(_headers);
	bool isStatusSet = false;

	while (getline(ss, line)) {
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);
		size_t colonPos = line.find(':');
		if (colonPos != string::npos) {
			string key = line.substr(0, colonPos);
			string value = line.substr(colonPos + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			if (toLower(key) == "status") {
				this->statusCode = atoi(value.c_str());
				isStatusSet = true;
			}
			else
				this->setHeader(key, value);
		}
	}
	if (!isStatusSet)
		this->statusCode = 200;

	this->setHeader("Content-Length", toString(this->body.size()));
}

void    CgiHandler::setupEnv() {
	vector<string> env_vars;
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
			env_vars.push_back("CONTENT_LENGTH=" + toString(this->request.getBody().size()));
		}
	}

	this->envp = new char*[env_vars.size() + 1];
	for (size_t i = 0; i < env_vars.size(); i++) {
		this->envp[i] = new char[env_vars[i].length() + 1];
		this->envp[i] = strcpy(this->envp[i], env_vars[i].c_str());
	}
	this->envp[env_vars.size()] = NULL;
}

void    closeFds(int* in, int *out) {
	if (in) {
		close(in[0]);
		close(in[1]);
	}
	if (out) {
		close(out[0]);
		close(out[1]);
	}
}

void    CgiHandler::executeChild() {
	if (chdir(this->resourceDir.c_str())) {
		closeFds(this->pipeIn, this->pipeOut);
		exit(EXIT_FAILURE);
	}

	dup2(this->pipeIn[0], STDIN_FILENO);
	dup2(this->pipeOut[1], STDOUT_FILENO);

	closeFds(this->pipeIn, this->pipeOut);

	char *argv[] = {
		(char*)this->scriptPath.c_str(),
		(char*)this->resourcePath.c_str(),
		NULL
	};

	execve(argv[0], argv, this->envp);

	exit(EXIT_FAILURE);
}

void    CgiHandler::executeParent() {
	close(this->pipeIn[0]);
	close(this->pipeOut[1]);

	write(this->pipeIn[1], this->request.getBody().data(), this->request.getBody().size());
	close(this->pipeIn[1]);

	char	buffer[4096];
	int		bytesRead;
	string	cgiOutput;
	while ((bytesRead = read(this->pipeOut[0], buffer, sizeof(buffer))) > 0)
		cgiOutput.append(buffer, bytesRead);
	
	close(this->pipeOut[0]);

	int status;
	waitpid(this->pid, &status, 0);

	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		this->statusCode = 502;
		return ;
	}
	populateResponse(cgiOutput);
}


void    CgiHandler::execute() {
	try {
		this->setupEnv();
		if (pipe(this->pipeIn) < 0 || pipe(this->pipeOut) < 0)
			throw runtime_error("CGI error: failed to create pipe!");

		this->pid = fork();
		if (this->pid < 0) 
			throw runtime_error("CGI error: fork() failed!");

		if (this->pid == 0)
			this->executeChild();
		else
			this->executeParent();
	} catch (const std::exception& e) {
		this->statusCode = 500;
	}
}

Response CgiHandler::exportResponse()
	{ return static_cast<Response&>(*this); }