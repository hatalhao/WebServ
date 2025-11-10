#include "../includes/cgi.hpp"

CgiHandler::CgiHandler(Client& client) :
	_client(client), envp(NULL), pid(-1), state(CGI_IDLE),
	writePipeFd(-1), readPipeFd(-1), output(""),
	bodyBytesWritten(0), expectedBodySize(0),
	sessionId(""), sessionCreated(false), startTime(0){}

CgiHandler::~CgiHandler() {
	if (envp) {
		for (int i = 0; envp[i] != NULL; i++) {
			delete[] envp[i];
		}
		delete[] envp;
	}
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

void    CgiHandler::addSessionToEnv(vector<string>& env_vars) {
	Request request = this->_client.getRequest();

	string cookieHeader = request.getHeader("cookie");
	if (!cookieHeader.empty()) {
		env_vars.push_back("HTTP_COOKIE=" + cookieHeader);
	}

	string existingSessionId = request.getCookie("session_id");
	if (!existingSessionId.empty()) {
		this->sessionId = existingSessionId;
		env_vars.push_back("SESSION_ID=" + existingSessionId);

		SessionManager& sessionManager = this->_client.getGlobalConfig()->getSessionManager();
		Session* session = sessionManager.getSession(existingSessionId);
		if (session)
			env_vars.push_back("SESSION_VALID=1");
		else 
			env_vars.push_back("SESSION_VALID=0");
	}

	env_vars.push_back("WEBSERV_SESSION_ENABLED=1");
}

string CgiHandler::createNewSession() {
	SessionManager& sessionManager = this->_client.getGlobalConfig()->getSessionManager();
	this->sessionId = sessionManager.createSession();
	this->sessionCreated = true;
	return this->sessionId;
}

void    CgiHandler::setupEnv() {
	Request request = this->_client.getRequest();
	vector<string> env_vars;
	env_vars.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env_vars.push_back("SERVER_SOFTWARE=WebServ/1.0");
	env_vars.push_back("SERVER_PROTOCOL=HTTP/1.1");
	env_vars.push_back("REQUEST_METHOD=" + request.getMethod());
	env_vars.push_back("QUERY_STRING=" + request.getQueryString());
	env_vars.push_back("SCRIPT_NAME=" + this->_client.getResponse().getResourcePath());
	env_vars.push_back("PYTHONUNBUFFERED=1");

	env_vars.push_back("PATH_INFO=" + this->_client.getResponse().getResourcePath());

	if (request.getMethod() == "POST") {
		env_vars.push_back("CONTENT_TYPE=" + request.getHeader("content-type"));
		if (request.getHeader("content-length") != "")
			env_vars.push_back("CONTENT_LENGTH=" + request.getHeader("content-length"));
		else {
			// Use body buffer size
			env_vars.push_back("CONTENT_LENGTH=" + toString(this->_client.getBodyBuffer().size()));
		}
	}

	this->addSessionToEnv(env_vars);

	this->envp = new char*[env_vars.size() + 1];
	for (size_t i = 0; i < env_vars.size(); i++) {
		this->envp[i] = new char[env_vars[i].length() + 1];
		this->envp[i] = strcpy(this->envp[i], env_vars[i].c_str());
	}
	this->envp[env_vars.size()] = NULL;
}

void    CgiHandler::executeChild(int *pipeIn, int *pipeOut) {

	Response response = this->_client.getResponse();
	if (chdir(response.getresourceDir().c_str())) {
		closeFds(pipeIn, pipeOut);
		exit(EXIT_FAILURE);
	}

	dup2(pipeIn[0], STDIN_FILENO);
	dup2(pipeOut[1], STDOUT_FILENO);
	
	// Redirect stderr to /dev/null to avoid buffering issues
	int devNull = open("/dev/null", O_WRONLY);
	if (devNull != -1) {
		dup2(devNull, STDERR_FILENO);
		close(devNull);
	}

	closeFds(pipeIn, pipeOut);

	string handlerPath = this->_client.getCgiHandlerPath();
	string resourcePath = response.getResourcePath();

	char *argv[] = {
		(char*)handlerPath.c_str(),
		(char*)resourcePath.c_str(),
		NULL
	};

	execve(argv[0], argv, this->envp);

	exit(EXIT_FAILURE);
}

bool	CgiHandler::start() {
	int pipeIn[2] = {-1}, pipeOut[2] = {-1};
	try {

		if (this->_client.getCgiHandlerPath().empty())
			return false;

		this->setupEnv();
		if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0)
			throw exception();

		this->pid = fork();
		if (this->pid < 0) throw exception();

		if (this->pid == 0)
			this->executeChild(pipeIn, pipeOut);

		close(pipeIn[0]);
		close(pipeOut[1]);

		writePipeFd = pipeIn[1];
		readPipeFd = pipeOut[0];

		Request _request = this->_client.getRequest();
		bool hasBody = (_request.getHeader("transfer-encoding") == "chunked"
						|| (_request.getHeader("content-length") != ""
								&&  _request.getHeader("content-length") != "0"));

		if (this->_client.getRequest().getMethod() == "POST" && hasBody) {
			this->state = CGI_WRITING;
			// Get expected body size
			string contentLengthHeader = _request.getHeader("content-length");
			if (!contentLengthHeader.empty()) {
				this->expectedBodySize = atoll(contentLengthHeader.c_str());
			}
			this->bodyBytesWritten = 0;
		}
		else {
			close(writePipeFd);
			writePipeFd = -1;
			this->state = CGI_READING;
		}
		this->startTime = time(NULL);
	} catch (...) {
		for (int i = 0; i < 2; ++i) {
			if (pipeIn[i] != -1) close(pipeIn[i]);
			if (pipeOut[i] != -1) close(pipeOut[i]);
		}
		this->_client.getResponse().setErrorResponse(500);
		return false;
	}
	return true;
}

void	CgiHandler::manageReadEvent(int epollFd) {
	if (this->state != CGI_READING || this->readPipeFd < 0)
	return;
	
	char	buffer[8192];
	ssize_t	bytesRead = read(this->readPipeFd, buffer, sizeof(buffer));
	
	if (bytesRead > 0) {
		this->output.append(buffer, bytesRead);
	} else if (bytesRead == 0) {
		epoll_ctl(epollFd, EPOLL_CTL_DEL, this->readPipeFd, NULL);
		close(this->readPipeFd);
		this->readPipeFd = -1;
		this->_client.onCgiDone(this->output);
		this->state = CGI_DONE;
	} else {
		this->state = CGI_DONE;
		epoll_ctl(epollFd, EPOLL_CTL_DEL, this->readPipeFd, NULL);
		close(this->readPipeFd);
		this->readPipeFd = -1;
		this->_client.onCgiError(502);
	}
}

void	CgiHandler::manageWriteEvent(int epollFd) {
	if (this->state != CGI_WRITING || this->writePipeFd < 0)
		return;

	if (expectedBodySize > 0 && bodyBytesWritten >= expectedBodySize) {
		epoll_ctl(epollFd, EPOLL_CTL_DEL, this->writePipeFd, 0);
		close(this->writePipeFd);
		this->writePipeFd = -1;
		this->state = CGI_READING;
		return;
	}

	const string& requestBuf = this->_client.getRequestBuf();
	
	if (requestBuf.empty())
		return;

	size_t toWrite = requestBuf.size();
	if (expectedBodySize > 0 && bodyBytesWritten + toWrite > expectedBodySize)
		toWrite = expectedBodySize - bodyBytesWritten;
	
	if (toWrite > 0) {
		ssize_t sent = write(this->writePipeFd, requestBuf.c_str(), toWrite);
		if (sent > 0) {
			bodyBytesWritten += sent;
			_client.consumeRequestBuf(sent);
			
			if (expectedBodySize > 0 && bodyBytesWritten >= expectedBodySize) {
				epoll_ctl(epollFd, EPOLL_CTL_DEL, this->writePipeFd, 0);
				close(this->writePipeFd);
				this->writePipeFd = -1;
				this->state = CGI_READING;
			}
		} else if (sent == 0) {
			this->state = CGI_DONE;
			epoll_ctl(epollFd, EPOLL_CTL_DEL, this->writePipeFd, 0);
			close(this->writePipeFd);
			this->writePipeFd = -1;
			this->_client.onCgiError(502);
		} else {
			this->state = CGI_DONE;
			epoll_ctl(epollFd, EPOLL_CTL_DEL, this->writePipeFd, 0);
			close(this->writePipeFd);
			this->writePipeFd = -1;
			this->_client.onCgiError(502);
		}
	}
}