#include "../../includes/client.hpp"

Client::Client(int fd) : socketFd(fd), contentLen(0), state(READING_REQUEST_LINE) {}

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

