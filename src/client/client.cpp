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
		fileBufferUsed = other.fileBufferUsed;
		fileBufferSent = other.fileBufferSent;
		fileReadOffset = other.fileReadOffset;
		memcpy(fileBuffer, other.fileBuffer, sizeof(fileBuffer));
	}
	return *this;
}

Client::Client(int fd, server* serverConfig) :
	_config(serverConfig), _clientFd(fd), requestBuf(""), responseBuf(""), contentLen(0),
	state(READING_REQUEST_LINE), request(_config->maxBodySize),
	response(serverConfig, request, httpStatusCode), httpStatusCode(0), bodySent(0), headerSent(0),
	fileBufferUsed(0), fileBufferSent(0), fileReadOffset(0) {}

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
        return;
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

        // Reset sent counters
        this->headerSent = 0;
        this->bodySent = 0;
        this->fileBufferUsed = 0;
        this->fileBufferSent = 0;
        this->fileReadOffset = 0;

        struct epoll_event event;
        memset(&event, 0, sizeof(event));
        event.data.fd = this->_clientFd;
        event.events = EPOLLOUT; 
        // if supporting pipelining: event.events |= EPOLLIN;

        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, this->_clientFd, &event) < 0) {
            perror("epoll_ctl MOD failed");
            return DISCONNECTED;
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