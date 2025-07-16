#include "../../includes/client.hpp"

void printState(ClientState state) {
    switch (state) {
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

void    Client::processRequest() {
    
    switch (this->state)
    {
    case READING_REQUEST_LINE:
        if (!this->request.processRequestLine(this->requestBuf))
            return ;
        else if (this->request.getState() == PARSINGHEADERS) {
            this->state = READING_HEADERS;
        } else if (this->request.getState() == PARSINGCOMPLETE) {
            this->state = GENERATING_RESPONSE;
        } else {
            // Handle error state
            this->state = SENDING_RESPONSE; // or some error handling state
        }
        printState(this->state);
        break;
    case READING_HEADERS:
        if (this->request.processHeaders(this->requestBuf) == 1) {
            this->state = READING_BODY; // or GENERATING_RESPONSE if no body expected
        } else if (this->request.getState() == PARSINGERROR) {
            // Handle error in headers parsing
            this->state = SENDING_RESPONSE; // or some error handling state
        }
        break;
    case READING_BODY:
        
        break;
    default:
        break;
    }
}