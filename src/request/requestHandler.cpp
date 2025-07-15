#include "../../includes/request.hpp"


void    Client::processRequest() {
    switch (this->state)
    {
    case READING_REQUEST_LINE:
        if (!this->Request.processRequestLine(this->requestBuf))
            return ;
        break;
    case READING_HEADERS:
        
        break;
    case READING_BODY:
        
        break;
    default:
        break;
    }
}