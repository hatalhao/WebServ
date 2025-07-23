#include "../../includes/client.hpp"

Client::Client(int fd) : socketFd(fd), contentLen(0), state(READING_REQUEST_LINE) {
    // Initialize the client with the given socket file descriptor
}

void	Client::printRequest() const {
    cout << "Request Method: " << request.getMethod() << endl;
    cout << "Request URI: " << request.getUri() << endl;
    cout << "Request Version: " << request.getVersion() << endl;
    for (map<string, string>::const_iterator it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it) {
        cout << it->first << ": " << it->second << endl;
    }
}

void	Client::printState() const {
    switch (this->state) {
        case READING_REQUEST_LINE:
            cout << "State: READING_REQUEST_LINE" << endl;
            break;
        case READING_HEADERS:
            cout << "State: READING_HEADERS" << endl;
            break;
        case READING_BODY:
            cout << "State: READING_BODY" << endl;
            break;
        case GENERATING_RESPONSE:
            cout << "State: GENERATING_RESPONSE" << endl;
            break;
        case SENDING_RESPONSE:
            cout << "State: SENDING_RESPONSE" << endl;
            break;
    }
}
