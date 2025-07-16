#include "../../includes/client.hpp"


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
            this->state = SENDING_RESPONSE; // or some error handling state
            cout << "bad request" << endl;
        }
        break;
    case READING_HEADERS:
        if (this->request.processHeaders(this->requestBuf) == 1) {
            if (this->request.getMethod() != GET)
                this->state = READING_BODY;
            else
                this->state =  GENERATING_RESPONSE;// or GENERATING_RESPONSE if no body expected
        } else if (this->request.getState() == PARSINGERROR) {
            // Handle error in headers parsing
            this->state = GENERATING_RESPONSE; // or some error handling state
        }
        break;
    case READING_BODY:
        // cout << "Processing body..." << endl;
        break;
    default:
        break;
    }
}