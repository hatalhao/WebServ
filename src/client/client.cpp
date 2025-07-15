#include "../../includes/client.hpp"

Client::Client(int fd) : socketFd(fd), contentLen(0), state(READING_REQUEST_LINE) {
    // Initialize the client with the given socket file descriptor
}