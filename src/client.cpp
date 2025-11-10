#include "../includes/client.hpp"

Client::Client(int fd, Worker* _g_config, const pair<string, int>& hostPort) :
	globalConfig(_g_config), _clientFd(fd),
	condidateServersIndices(_g_config->getCondidateServersIndices(hostPort)),
	serverConfig(&_g_config->getServers()[*condidateServersIndices.begin()]),
	requestBuf(""), responseBuf(""), contentLen(0), state(READING_REQUEST_LINE),
	request(), response(this), cgiHandlerPath(""),
	httpStatusCode(0), keepAlive(false), lastActivity(time(NULL)),
	uploadFd(-1),
	bodySent(0), headerSent(0), bodyBytesProcessed(0),
	fileBufferUsed(0), fileBufferSent(0), fileReadOffset(0) {}

void    Client::resolveHost() {
	string	requestServerName = this->request.getHeader("host");
	size_t colon = this->request.getHeader("host").find(':');
	if (colon != string::npos)
    	requestServerName = this->request.getHeader("host").substr(0, colon);
	vector<server>& servers = this->globalConfig->getServers();
	for (set<int>::iterator it = condidateServersIndices.begin(); it != condidateServersIndices.end(); ++it) {
		if (find(servers[*it].serverName.begin(), servers[*it].serverName.end(),
				requestServerName) != servers[*it].serverName.end())
		{	this->serverConfig = &servers[*it]; return; }
	}
	this->serverConfig = &servers[*condidateServersIndices.begin()];
}

void    Client::updateState() {
	switch (this->request.getState()) {
		case PARSINGREQUESTLINE:
			this->state = READING_REQUEST_LINE; break;
		case REQUESTLINE_READY:
			this->state = READING_HEADERS; break;
		case REQUESTHEADERS_READY:
			this->resolveHost();
			// Headers are complete - mark as done and let handler process body from socket
			this->state = READING_DONE;
			break;
		case PARSINGCOMPLETE:
			this->state = READING_DONE; break;
		case PARSINGERROR:
			this->state = READING_BAD_REQUEST; break;
		default:
			this->state = READING_DONE; break;
	}
}

void    Client::processRequest() {
	ClientState previousState = this->state;
	
	switch (this->state)
	{
		case READING_REQUEST_LINE: 
			this->httpStatusCode = this->request.processRequestLine(this->requestBuf);
			break;
		case READING_HEADERS:
			this->httpStatusCode = this->request.processHeaders(this->requestBuf);
			break;
		default:
			break;
	}

	updateState();

	if (this->state != previousState && this->state != READING_DONE
		&& this->state != READING_BAD_REQUEST)
		this->processRequest();
	
	if (this->state == READING_DONE) {
		string connectionHeader = this->request.getHeader("connection");
		for (size_t i = 0; i < connectionHeader.length(); i++)
			connectionHeader[i] = tolower(connectionHeader[i]);
		this->keepAlive = (connectionHeader == "keep-alive");
	}
}

void    Client::resetWriteState() {
	this->headerSent = 0;
	this->bodySent = 0;
	this->fileBufferUsed = 0;
	this->fileBufferSent = 0;
	this->fileReadOffset = 0;
}

void    Client::resetForNextRequest() {
	this->requestBuf.clear();
	this->responseBuf.clear();
	this->cgiHandlerPath.clear();
	this->httpStatusCode = 0;
	this->contentLen = 0;
	this->state = READING_REQUEST_LINE;
	
	if (this->uploadFd >= 0) {
		close(this->uploadFd);
		this->uploadFd = -1;
	}
	this->bodyBytesProcessed = 0;
	
	this->resetWriteState();

	this->request.~Request();
	new (&this->request) Request();
	
	this->response.~Response();
	new (&this->response) Response(this);

	this->lastActivity = time(NULL);
}


void	Client::forwardToCgi() {
	cgiHandlerPath = this->response.searchCgiHandlerPath();
	CgiHandler* cgi = new CgiHandler(*this);
	if (cgi->start()) {
		this->globalConfig->getCgiSet().insert(cgi);
		struct epoll_event ev = epoll_event();
		ev.events = 0;

		if (cgi->getState() == CGI_WRITING) {
			ev.events |= EPOLLOUT; 
			ev.data.fd = cgi->getWriteFd();
			this->globalConfig->getCgiHandlers()[ev.data.fd] = cgi;
			epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_ADD, ev.data.fd, &ev);
		}
		ev.events = EPOLLIN; 
		ev.data.fd = cgi->getReadFd();
		this->globalConfig->getCgiHandlers()[ev.data.fd] = cgi;
		epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_ADD, ev.data.fd, &ev);

		this->state = WAITING_FOR_CGI;
	}
	else {
		delete cgi;
		if (!httpStatusCode) httpStatusCode = 500;
		response.setErrorResponse(httpStatusCode);
		responseBuf = response.serializeHeadersAndBody();
		state = SENDING_RESPONSE;
	}
}

void    Client::generateResponse() {
	if (httpStatusCode != 0 || response.prepareResponse()) {
		response.setErrorResponse(httpStatusCode);
		responseBuf = response.serializeHeadersAndBody();
		resetWriteState();
		state = SENDING_RESPONSE;
		return;
	}

	if (this->response.checkCgiRequest())
		return forwardToCgi();

	// For POST with body, prepare upload and set state to PROCESSING_UPLOAD
	if (request.getMethod() == "POST") {
		if (response.preparePostDestination()) {
			uploadFd = response.bodyFd;
			bodyBytesProcessed = 0;
			
			// Check if there's body to process
			string contentLengthHeader = request.getHeader("content-length");
			string transferEncoding = request.getHeader("transfer-encoding");
			
			if (!contentLengthHeader.empty() || transferEncoding == "chunked") {
				size_t contentLength = contentLengthHeader.empty() ? 0 : atoll(contentLengthHeader.c_str());
				
				if (!requestBuf.empty() && uploadFd >= 0) {
					size_t toWrite = requestBuf.size();
					if (contentLength > 0 && toWrite > contentLength) {
						toWrite = contentLength;
					}
					
					if (toWrite > 0) {
						ssize_t written = write(uploadFd, requestBuf.data(), toWrite);
						if (written > 0) {
							bodyBytesProcessed += written;
							requestBuf.clear();
						} else {
							close(uploadFd);
							uploadFd = -1;
							httpStatusCode = 500;
						}
					}
				}
				
				// Check if upload already complete
				if (contentLength > 0 && bodyBytesProcessed >= contentLength) {
					close(uploadFd);
					uploadFd = -1;
					response.finalizePost();
					
					if (!response.isFileBody) {
						if (response.findHeader("Content-Length") == false)
							response.setHeader("Content-Length", to_string(response.bodySize));
						responseBuf = response.serializeHeadersAndBody();
					} else
						responseBuf = response.serializeHeaders();

					resetWriteState();
					state = SENDING_RESPONSE;
					return;
				}
				
				state = PROCESSING_UPLOAD;
				// Remaining body will be read in handleReadEvent
				return;
			}
			// No body, finalize immediately
			response.finalizePost();
		}
	}
	else if (request.getMethod() == "GET")
		response.Get();
	else if (request.getMethod() == "DELETE")
		response.Delete();

	if (!response.isFileBody) {
		if (response.findHeader("Content-Length") == false)
			response.setHeader("Content-Length", to_string(response.bodySize));
		responseBuf = response.serializeHeadersAndBody();
	} else
		responseBuf = response.serializeHeaders();

	resetWriteState();
	state = SENDING_RESPONSE;
}


int Client::handleReadEvent(int epollFd) {
	char buffer[8092];
	ssize_t bytesRead;

	bytesRead = recv(this->_clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead > 0) {
		this->updateActivity();
		
		if (state == WAITING_FOR_CGI) {
			if (this->requestBuf.size() < 16384) {
				size_t toBuffer = bytesRead;
				if (this->requestBuf.size() + bytesRead > 16384)
					toBuffer = 16384 - this->requestBuf.size();
				this->requestBuf.append(buffer, toBuffer);
			}
			return CONTINUE;
		}

		if (state == PROCESSING_UPLOAD) {
			if (uploadFd >= 0) {
				string contentLengthHeader = request.getHeader("content-length");
				size_t contentLength = contentLengthHeader.empty() ? 0 : atoll(contentLengthHeader.c_str());
				
				size_t toWrite = bytesRead;
				if (contentLength > 0 && bodyBytesProcessed + bytesRead > contentLength) {
					toWrite = contentLength - bodyBytesProcessed;
				}
				
				if (toWrite > 0) {
					ssize_t written = write(uploadFd, buffer, toWrite);
					if (written > 0) {
						bodyBytesProcessed += written;
					} else {
						close(uploadFd);
						uploadFd = -1;
						httpStatusCode = 500;
						response.setErrorResponse(httpStatusCode);
						responseBuf = response.serializeHeadersAndBody();
						resetWriteState();
						state = SENDING_RESPONSE;
						
						struct epoll_event event = epoll_event();
						event.data.fd = this->_clientFd;
						event.events = EPOLLOUT;
						epoll_ctl(epollFd, EPOLL_CTL_MOD, this->_clientFd, &event);
						return CONTINUE;
					}
				}
				
				if (contentLength > 0 && bodyBytesProcessed >= contentLength) {
					close(uploadFd);
					uploadFd = -1;
					response.finalizePost();
					
					if (!response.isFileBody) {
						if (response.findHeader("Content-Length") == false)
							response.setHeader("Content-Length", to_string(response.bodySize));
						responseBuf = response.serializeHeadersAndBody();
					} else
						responseBuf = response.serializeHeaders();

					resetWriteState();
					state = SENDING_RESPONSE;
					
					struct epoll_event event = epoll_event();
					event.data.fd = this->_clientFd;
					event.events = EPOLLOUT;
					epoll_ctl(epollFd, EPOLL_CTL_MOD, this->_clientFd, &event);
				}
			}
			return CONTINUE;
		}
		this->requestBuf.append(buffer, bytesRead);
	}
	else if (bytesRead == 0)
		return DISCONNECTED;
	else if (bytesRead < 0)
		return CONTINUE;

	processRequest();
	if (state == READING_DONE || state == READING_BAD_REQUEST) {
		generateResponse();

		if (state == SENDING_RESPONSE) {
			struct epoll_event event = epoll_event();
			event.data.fd = this->_clientFd;
			event.events = EPOLLOUT;
	
			if (epoll_ctl(epollFd, EPOLL_CTL_MOD, this->_clientFd, &event) < 0)
				return DISCONNECTED;
		}
	}

	return CONTINUE;
}

int	Client::endStreamingFile(size_t n) {
	if (n == 0 || response.bodySent >= response.bodySize) {
		close(response.bodyFd);
		response.isFileBody = false;
		if (keepAlive)
			return (resetForNextRequest(), FINISHED);
		else
			return DISCONNECTED;
	}
	return -2;
}

int Client::streamFileResponse(ssize_t& sent) {
	if (fileBufferUsed == fileBufferSent) {
		size_t remaining = response.bodySize - fileReadOffset;
		int ret = endStreamingFile(remaining);
		if ( ret != -2) return ret;

		size_t toRead = (remaining < sizeof(fileBuffer) ? remaining : sizeof(fileBuffer));
		ssize_t bytesRead = read(response.bodyFd, fileBuffer, toRead);

		if (bytesRead < 0) return DISCONNECTED;
		if (bytesRead == 0) return DISCONNECTED;

		ret = endStreamingFile(bytesRead);
		if (ret != -2) return ret;
	
		fileBufferUsed = bytesRead;
		fileBufferSent = 0;
		fileReadOffset += bytesRead;
	}

	size_t toSend = fileBufferUsed - fileBufferSent;
	sent = send(_clientFd, fileBuffer + fileBufferSent, toSend, 0);

	if (sent < 0)
		return CONTINUE;
	if (sent == 0)
		return DISCONNECTED;

	fileBufferSent += sent;
	response.bodySent += sent;
	this->updateActivity();
	return CONTINUE;
}

int Client::handleWriteEvent() {
	ssize_t sent;

	if (headerSent < responseBuf.size()) {
		const char* buf = responseBuf.c_str() + headerSent;
		size_t toSend = responseBuf.size() - headerSent;

		sent = send(_clientFd, buf, toSend, 0);
		if (sent < 0)
			return CONTINUE;
		if (sent == 0)
			return DISCONNECTED;

		headerSent += sent;
		this->updateActivity();
		if (headerSent < responseBuf.size())
			return CONTINUE;
	}
	if (response.isFileBody)
		return streamFileResponse(sent);

	if (keepAlive)
		return (resetForNextRequest(), FINISHED);

	return DISCONNECTED;
}

string	strToLower(string& src) {
	string _new = src;
	for (size_t i = 0; i < _new.length(); i++)
		_new[i] = tolower(_new[i]);
	return (_new);
}

int	Client::parseCgiOutput(string& cgiOutput) {

	if (cgiData.crlfPos != string::npos) {
		cgiData._headers = cgiOutput.substr(0, cgiData.crlfPos);
		cgiData._body = cgiOutput.substr(cgiData.crlfPos + CRLF_LENGTH + CRLF_LENGTH); 
        cgiData.headersFound = true;
	}
	else if (cgiData.lfPos != string::npos) {
		cgiData._headers = cgiOutput.substr(0, cgiData.lfPos);
		cgiData._body = cgiOutput.substr(cgiData.lfPos + 2); 
        cgiData.headersFound = true;
	}
	else
        return (onCgiError(502), -1);

	this->response.setBody(cgiData._body);

	stringstream ss(cgiData._headers);
	string newSessionId, deleteSessionId, line;

	while (getline(ss, line)) {
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);
		size_t colonPos = line.find(':');
		if (colonPos != string::npos) {
			string key = line.substr(0, colonPos);
			string value = line.substr(colonPos + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			
			if (strToLower(key) == "status") {
				this->httpStatusCode = atoi(value.c_str());
				cgiData.isStatusSet = true;
			}
			else if (strToLower(key) == "x-webserv-create-session")
				cgiData.sessionRequested = true;
			else if (strToLower(key) == "x-webserv-session-id")
				newSessionId = value;
			else if (strToLower(key) == "x-webserv-delete-session")
				deleteSessionId = value;
			else
				this->response.setHeader(key, value);
		}
	}
	checkSession(newSessionId, deleteSessionId);
	return 0;
}

void	Client::checkSession(string& newSID, string& delSID) {
	if (cgiData.sessionRequested) {
		if (newSID.empty()) {
			SessionManager& sessionManager = this->globalConfig->getSessionManager();
			newSID = sessionManager.createSession();
		}
		string cookieValue = "session_id=" + newSID + "; HttpOnly; Path=/";
		this->response.setHeader("Set-Cookie", cookieValue);
	}
	
	if (!delSID.empty()) {
		SessionManager& sessionManager = this->globalConfig->getSessionManager();
		sessionManager.deleteSession(delSID);
		string cookieValue = "session_id=; HttpOnly; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT";
		this->response.setHeader("Set-Cookie", cookieValue);
	}
}

void	Client::onCgiDone(string& cgiOutput) {

	cgiData.crlfPos = cgiOutput.find(CRLF CRLF);
	cgiData.lfPos = cgiOutput.find("\n\n");
    cgiData.headersFound = false;
    cgiData.isStatusSet = false;

	if (parseCgiOutput(cgiOutput)) return ;

	if (!cgiData.isStatusSet)
		this->httpStatusCode = (cgiData.headersFound ? 200 : 403);

	this->response.setHeader("Content-Length", toString(this->response.getBodySize()));

	this->responseBuf = this->response.serializeHeadersAndBody();
	this->response.bodySent = 0;

	this->state = SENDING_RESPONSE;

	struct epoll_event event = epoll_event();
	event.data.fd = this->_clientFd;
	event.events = EPOLLOUT;
	epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_MOD, this->_clientFd, &event);
	
}

void    Client::onCgiError(unsigned short code) {
	this->httpStatusCode = code;
	this->response.setErrorResponse(this->httpStatusCode);

	this->responseBuf = this->response.serializeHeadersAndBody();
	this->response.bodySent = 0;

	this->state = SENDING_RESPONSE;

	struct epoll_event event = epoll_event();
	event.data.fd = this->_clientFd;
	event.events = EPOLLOUT;
	epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_MOD, this->_clientFd, &event);
}

