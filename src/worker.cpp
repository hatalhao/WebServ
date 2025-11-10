#include "../includes/worker.hpp"

Worker* Worker::instance = 0;

Worker::~Worker(){
	for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		close(it->first);
		delete it->second;
	}
	_clients.clear();
	
	for (std::set<CgiHandler*>::iterator it = _cgiSet.begin(); it != _cgiSet.end(); ++it) {
		CgiHandler* handler = *it;
		if (handler) {
			pid_t pid = handler->getPid();
			if (pid > 0)
				kill(pid, SIGKILL);
			
			int rfd = handler->getReadFd();
			int wfd = handler->getWriteFd();
			if (rfd >= 0) close(rfd);
			if (wfd >= 0) close(wfd);
			
			delete handler;
		}
	}
	_cgiSet.clear();
	_cgiHandlers.clear();

	if (_epollFd >= 0) {
		close(_epollFd);
	}
	
	this->socketToServer.clear();
	this->hostToIndex.clear();
};

void Worker::cleanup() {
    delete Worker::instance;
    Worker::instance = 0;
}

Worker* Worker::getInstance(const std::string& filename) {
	if (!Worker::instance)
		Worker::instance = new Worker(filename);
	return instance;
}