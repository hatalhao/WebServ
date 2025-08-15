#include "../includes/client.hpp"

void	Client::printRequest() const {
    cout << "Request Method: " << this->request.getMethod();
    cout << endl;
    cout << "Request Path: [" << request.getPath() << "]" << endl;
    cout << "Request Query: [" << request.getQueryString() << "]" << endl;
    cout << "Request Version: [" << request.getVersion() << "]" << endl;
    for (map<string, string>::const_iterator it = request.getHeaders().begin(); it != request.getHeaders().end(); ++it) {
        cout << it->first << ": [" << it->second << "]"<< endl;
    }
    cout << "--Body:\n" << BRIGHT_GREEN "[" RESET; 
    for (vector<char>::const_iterator it = request.getBody().begin(); it != request.getBody().end(); it++) {
        cout << *it;
    }
    cout << BRIGHT_GREEN "]" RESET << endl;
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
        case READING_DONE:
            cout << "State: READING_DONE" << endl;
            break;
        case READING_BAD_REQUEST:
            cout << "State: READING_BAD_REQUEST" << endl;
            break;
        case GENERATING_RESPONSE:
            cout << "State: GENERATING_RESPONSE" << endl;
            break;
        case SENDING_RESPONSE:
            cout << "State: SENDING_RESPONSE" << endl;
            break;
    }
    cout << BRIGHT_GREEN << this->httpStatusCode << RESET << endl;
}
