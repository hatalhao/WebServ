#include "../../includes/client.hpp"

Client::Client(int fd, Config* _g_config) :
	globalConfig(_g_config), _clientFd(fd), serverConfig(NULL),
	requestBuf(""), responseBuf(""), contentLen(0), state(READING_REQUEST_LINE),
	request(this), response(this), cgiHandlerPath(""),
	httpStatusCode(0), bodySent(0), headerSent(0),
	fileBufferUsed(0), fileBufferSent(0), fileReadOffset(0) {}

void    Client::resolveHost() {
	string	requestServerName = this->request.getHeader("host");
	size_t colon = this->request.getHeader("host").find(':');
	if (colon != string::npos)
    	requestServerName = this->request.getHeader("host").substr(0, colon);
	for (vector<server>::iterator it = this->globalConfig->_servers.begin();
								it != this->globalConfig->_servers.end(); ++it) {
		if (find(it->serverName.begin(),
				it->serverName.end(),
				requestServerName) != it->serverName.end())
		{	this->serverConfig = &*it; return; }
	}
	this->serverConfig = &this->globalConfig->_servers[0];
}

void    Client::updateState() {
	switch (this->request.getState()) {
		case PARSINGREQUESTLINE:
			this->state = READING_REQUEST_LINE; break;
		case REQUESTLINE_READY:
			this->state = READING_HEADERS; break;
		case REQUESTHEADERS_READY:
			this->state = (this->request.getMethod() == "POST") ?
							READING_BODY : READING_DONE;
			this->resolveHost();
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
		case READING_BODY:
			this->httpStatusCode = this->request.processBody(this->requestBuf);
			break;
		default:
			break;
	}

	updateState();

	if (this->state != previousState && this->state != READING_DONE
		&& this->state != READING_BAD_REQUEST)
		this->processRequest();
}

void    Client::resetWriteState() {
	this->headerSent = 0;
	this->bodySent = 0;
	this->fileBufferUsed = 0;
	this->fileBufferSent = 0;
	this->fileReadOffset = 0;
}

void    Client::generateResponse() {
	if (httpStatusCode != 0 || response.prepareResponse()) {
		response.setErrorResponse(httpStatusCode);
		responseBuf = response.serializeHeadersAndBody();
		resetWriteState();
		state = SENDING_RESPONSE;
		return;
	}

	if (this->response.searchCgiHandlerPath().empty() == false) {
		this->cgiHandlerPath = this->response.searchCgiHandlerPath();
		CgiHandler* cgi = new CgiHandler(*this);
		if (cgi->start()) {
			// Track handler lifetime to avoid dangling pointers during cleanup
			this->globalConfig->_cgiSet.insert(cgi);
			struct epoll_event ev = epoll_event();
			// Level-triggered events by default
			ev.events = 0;

			// If the CGI needs to be written-to (POST), watch for EPOLLOUT on its write-pipe.
			if (cgi->getState() == CGI_WRITING) {
				ev.events |= EPOLLOUT; // watch write readiness
				ev.data.fd = cgi->getWriteFd();
				this->globalConfig->_cgiHandlers[ev.data.fd] = cgi;
				epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_ADD, ev.data.fd, &ev);
			}
			ev.events = EPOLLIN; // watch read readiness
			ev.data.fd = cgi->getReadFd();
			this->globalConfig->_cgiHandlers[ev.data.fd] = cgi;
			epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_ADD, ev.data.fd, &ev);

			this->state = WAITING_FOR_CGI;
		}
		else {
			delete cgi;
			httpStatusCode = 500;
			response.setErrorResponse(httpStatusCode);
			responseBuf = response.serializeHeadersAndBody();
			state = SENDING_RESPONSE;
		}
		return ;
	}

	if (request.getMethod() == "GET")
		httpStatusCode = response.Get();
	else if (request.getMethod() == "POST")
		httpStatusCode = response.Post();
	else if (request.getMethod() == "DELETE")
		httpStatusCode = response.Delete();

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
	char buffer[4096];
	ssize_t bytesRead;

	bytesRead = recv(this->_clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead > 0) {
		this->requestBuf.append(buffer, bytesRead);
	} else if (bytesRead == 0) {
		return DISCONNECTED; // client closed
	} else if (bytesRead < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("recv failed");
			return DISCONNECTED;
		}
	}

	this->processRequest();
	if (this->state == READING_DONE || this->state == READING_BAD_REQUEST) {
		this->generateResponse();

		if (this->state == SENDING_RESPONSE) {
			struct epoll_event event = epoll_event();
			event.data.fd = this->_clientFd;
			event.events = EPOLLOUT; // level-triggered write readiness
	
			if (epoll_ctl(epollFd, EPOLL_CTL_MOD, this->_clientFd, &event) < 0) {
				perror("epoll_ctl MOD failed");
				return DISCONNECTED;
			}
		}
	}

	return CONTINUE;
}


int Client::handleWriteEvent() {
	ssize_t sent;

	// First, send remaining headers
	if (headerSent < responseBuf.size()) {
		const char* buf = responseBuf.c_str() + headerSent;
		size_t toSend = responseBuf.size() - headerSent;

		sent = send(_clientFd, buf, toSend, 0);
		if (sent < 0) {
			if (errno == EAGAIN) return CONTINUE;
			return DISCONNECTED;
		}

		headerSent += sent;
		if (headerSent < responseBuf.size())
			return CONTINUE; // still sending headers
	}
	// If it's a file, stream it in chunks
	if (response.isFileBody) {
		// If buffer is empty or fully sent, read more data
		if (fileBufferUsed == fileBufferSent) {
			size_t remaining = response.bodySize - fileReadOffset;
			if (remaining == 0 || response.bodySent >= response.bodySize) {
				// All data sent
				close(response.bodyFd);
				response.isFileBody = false;
				state = READING_REQUEST_LINE;
				headerSent = 0;
				bodySent = 0;
				fileBufferUsed = 0;
				fileBufferSent = 0;
				fileReadOffset = 0;
				return FINISHED;
			}

			size_t chunk = remaining < sizeof(fileBuffer) ? remaining : sizeof(fileBuffer);
			ssize_t bytesRead = pread(response.bodyFd, fileBuffer, chunk, fileReadOffset);
			
			if (bytesRead < 0) {
				if (errno == EAGAIN) return CONTINUE;
				return DISCONNECTED;
			} else if (bytesRead == 0) {
				// EOF - check if we've sent everything
				if (response.bodySent >= response.bodySize) {
					close(response.bodyFd);
					response.isFileBody = false;
					state = READING_REQUEST_LINE;
					headerSent = 0;
					bodySent = 0;
					fileBufferUsed = 0;
					fileBufferSent = 0;
					fileReadOffset = 0;
					return FINISHED;
				}
				return DISCONNECTED; // Unexpected EOF
			}

			fileBufferUsed = bytesRead;
			fileBufferSent = 0;
			fileReadOffset += bytesRead;
		}

		// Send remaining data from buffer
		size_t toSend = fileBufferUsed - fileBufferSent;
		sent = send(_clientFd, fileBuffer + fileBufferSent, toSend, 0);
		
		if (sent < 0) {
			if (errno == EAGAIN) return CONTINUE;
			return DISCONNECTED;
		}

		fileBufferSent += sent;
		response.bodySent += sent;

		// If buffer not fully sent, continue next time
		if (fileBufferSent < fileBufferUsed)
			return CONTINUE;

		// Buffer fully sent, will read more next time
		return CONTINUE;
	}

	// If not a file and headers were already sent
	state = READING_REQUEST_LINE;
	headerSent = 0;
	bodySent = 0;
	return FINISHED;
}


string	strToLower(string& src) {
	string _new = src;
	for (size_t i = 0; i < _new.length(); i++)
		_new[i] = tolower(_new[i]);
	return (_new);
}

void	Client::onCgiDone(string& cgiOutput) {

	size_t crlfPos = cgiOutput.find(CRLF CRLF);
	size_t lfPos = cgiOutput.find("\n\n");
	string	_headers, _body, line;

	if (crlfPos != string::npos) {
		_headers = cgiOutput.substr(0, crlfPos);
		_body = cgiOutput.substr(crlfPos + CRLF_LENGTH + CRLF_LENGTH); 
	}
	else if (lfPos != string::npos) {
		_headers = cgiOutput.substr(0, lfPos);
		_body = cgiOutput.substr(lfPos + 2); 
	}
	else 
		_body = cgiOutput;

	this->response.setBody(_body);

	stringstream ss(_headers);
	bool isStatusSet = false;
	bool sessionRequested = false;
	string newSessionId;
	string deleteSessionId;

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
				isStatusSet = true;
			}
			else if (strToLower(key) == "x-webserv-create-session") {
				// CGI script requests session creation
				sessionRequested = true;
			}
			else if (strToLower(key) == "x-webserv-session-id") {
				// CGI script provides session ID to use
				newSessionId = value;
			}
			else if (strToLower(key) == "x-webserv-delete-session") {
				// CGI script requests session deletion
				deleteSessionId = value;
			}
			else
				this->response.setHeader(key, value);
		}
	}
	
	// Handle session creation if requested
	if (sessionRequested) {
		if (newSessionId.empty()) {
			SessionManager& sessionManager = this->globalConfig->getSessionManager();
			newSessionId = sessionManager.createSession();
		}
		// Set session cookie
		string cookieValue = "session_id=" + newSessionId + "; HttpOnly; Path=/";
		this->response.setHeader("Set-Cookie", cookieValue);
	}
	
	// Handle session deletion if requested
	if (!deleteSessionId.empty()) {
		SessionManager& sessionManager = this->globalConfig->getSessionManager();
		sessionManager.deleteSession(deleteSessionId);
	}
	if (!isStatusSet)
		this->httpStatusCode = 200;

	this->response.setHeader("Content-Length", toString(this->response.getBodySize()));
		// --- 2. Serialize the final response into the buffer ---
	this->responseBuf = this->response.serializeHeadersAndBody();
	this->response.bodySent = 0;

	// --- 3. CRUCIAL: Transition the client's state to WRITING ---
	this->state = SENDING_RESPONSE;

	// --- 4. Tell epoll to watch for when this client's socket is ready for writing ---
	struct epoll_event event = epoll_event();
	event.data.fd = this->_clientFd;
	event.events = EPOLLOUT; // level-triggered
	epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_MOD, this->_clientFd, &event);
	
}

void    Client::onCgiError(unsigned short code) {
	this->httpStatusCode = code;
	this->response.setErrorResponse(this->httpStatusCode);

	// 2. Serialize the error response into the buffer.
	this->responseBuf = this->response.serializeHeadersAndBody();
	this->response.bodySent = 0;

	// 3. Transition the client's state to WRITING to send the error page.
	this->state = SENDING_RESPONSE;

	// 4. Tell epoll to watch for when this client's socket is ready for writing.
	struct epoll_event event = epoll_event();
	event.data.fd = this->_clientFd;
	event.events = EPOLLOUT; // level-triggered
	epoll_ctl(this->globalConfig->getEpollFd(), EPOLL_CTL_MOD, this->_clientFd, &event);
}
