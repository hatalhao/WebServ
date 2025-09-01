#include "../../includes/client.hpp"

Client& Client::operator=(const Client& other) {
	if (this != &other) {
		_config = other._config;
		requestBuf = other.requestBuf;
		responseBuf = other.responseBuf;
		contentLen = other.contentLen;
		state = other.state;
		request = other.request;
		response = other.response;
		httpStatusCode = other.httpStatusCode;
	}
	return *this;
}

Client::Client(int fd, server* serverConfig) :
	_config(serverConfig), _clientFd(fd), requestBuf(""), responseBuf(""), contentLen(0),
	state(READING_REQUEST_LINE), request(_config->maxBodySize),
	response(serverConfig, request, httpStatusCode), httpStatusCode(0), bytesSent(0) {}

void    Client::updateState() {
	switch (this->request.getState()) {
		case PARSINGREQUESTLINE:
			this->state = READING_REQUEST_LINE; break;
		case REQUESTLINE_READY:
			this->state = READING_HEADERS; break;
		case REQUESTHEADERS_READY:
			this->state = (this->request.getMethod() == "POST") ?
							READING_BODY : READING_DONE;
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

void    Client::generateResponse() {
	if (this->httpStatusCode != 0) {
		this->responseBuf = this->response.serializeResponse(); 
		return;
	}

	this->httpStatusCode = response.prepareResponse();

	if (this->request.getMethod() == "GET") {
		this->httpStatusCode = response.Get();
	}
	else if (this->request.getMethod() == "POST") {
		this->httpStatusCode = this->response.Post();
	}
	else if (this->request.getMethod() == "DELETE") {
		
	}
	this->responseBuf = this->response.serializeResponse();
}


int Client::handleReadEvent(int epollFd) {
    char buffer[4096];
    ssize_t bytesRead;

    // Read until EAGAIN because EPOLLET
    while ((bytesRead = recv(this->_clientFd, buffer, sizeof(buffer), 0)) > 0) {
        this->requestBuf.append(buffer, bytesRead);
    }

    if (bytesRead == 0) {
        return DISCONNECTED; // client closed
    }
    if (bytesRead < 0 && errno != EAGAIN) {
        perror("recv failed");
        return DISCONNECTED;
    }

    this->processRequest();

    if (this->state == READING_DONE || this->state == READING_BAD_REQUEST) {
        this->generateResponse();
        this->state = SENDING_RESPONSE;
        this->bytesSent = 0;

        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.data.fd = this->_clientFd;
        event.events = EPOLLOUT | EPOLLET; 
        // if supporting pipelining: event.events |= EPOLLIN;

        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, this->_clientFd, &event) < 0) {
            perror("epoll_ctl MOD failed");
            return DISCONNECTED;
        }
    }

    return CONTINUE;
}


int Client::handleWriteEvent(int epollFd) {
    if (this->state != SENDING_RESPONSE)
        return CONTINUE;

    size_t toSend = this->responseBuf.size() - this->bytesSent;
    const char* buf = this->responseBuf.c_str() + this->bytesSent;

    ssize_t sent = send(this->_clientFd, buf, toSend, 0);

    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return CONTINUE; // try again later
        }
        perror("send failed");
        return DISCONNECTED;
    }

    this->bytesSent += sent;

    if (this->bytesSent == this->responseBuf.size()) {
        // Done sending response
        this->responseBuf.clear();
        this->state = READING_REQUEST_LINE; // reset for next request if keep-alive

        // Switch back to EPOLLIN (or EPOLLIN|EPOLLET if edge triggered)
        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.data.fd = this->_clientFd;
        event.events = EPOLLIN | EPOLLET;

        epoll_ctl(epollFd, EPOLL_CTL_MOD, this->_clientFd, &event);

        return FINISHED;
    }

    return CONTINUE;
}