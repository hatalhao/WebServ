#include "../../includes/client.hpp"

Client::Client(int fd, const Config* configuration) :
    socketFd(fd), contentLen(0), state(READING_REQUEST_LINE),
    request(configuration) , httpStatusCode(0) {}

void    Client::updateState() {
    switch (this->request.getState()) {
        case PARSINGREQUESTLINE:
            this->state = READING_REQUEST_LINE; break;
        case REQUESTLINE_READY:
            this->state = READING_HEADERS; break;
        case REQUESTHEADERS_READY:
            this->state = (this->request.getMethod() == POST) ?
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
        break;
    default:
        break;
    }
    updateState();
}