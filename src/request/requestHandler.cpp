#include "../../includes/client.hpp"

void    Client::processRequest() {

    switch (this->state)
    {
    case READING_REQUEST_LINE: 
        this->request.processRequestLine(this->requestBuf); break;
    case READING_HEADERS:
        this->request.processHeaders(this->requestBuf); break;
    case READING_BODY:
        // cout << "Processing body..." << endl;
        break;
    default:
        break;
    }
    updateState();
}