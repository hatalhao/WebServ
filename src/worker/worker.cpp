#include "../../includes/worker.hpp"

Worker* Worker::instance = 0;

Worker::~Worker(){
	for (size_t i = 0; i < this->_clients.size(); ++i) { delete this->_clients[i]; }
	this->socketToServer.erase(this->socketToServer.begin(), this->socketToServer.end());
	this->clientToServer.erase(this->clientToServer.begin(), this->clientToServer.end());
	this->hostToIndex.erase(this->hostToIndex.begin(), this->hostToIndex.end());
	this->_clients.clear();
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