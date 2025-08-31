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
	_config(serverConfig), currentPort(fd), requestBuf(""), responseBuf(""), contentLen(0),
	state(READING_REQUEST_LINE), request(_config->maxBodySize),
	response(serverConfig, request, httpStatusCode), httpStatusCode(0) {}

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
	switch (this->state)
	{
	case READING_REQUEST_LINE: 
		this->httpStatusCode = this->request.processRequestLine(this->requestBuf); break;
	case READING_HEADERS:
		this->httpStatusCode = this->request.processHeaders(this->requestBuf); break;
	case READING_BODY:
		this->httpStatusCode = this->request.processBody(this->requestBuf); break;
		break;
	default:
		break;
	}
	updateState();
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