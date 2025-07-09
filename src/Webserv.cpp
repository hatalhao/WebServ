#include "../includes/Webserv.hpp"

void	Webserv::checkAttributes() const  {
	if (port < 1 || port > 65535)
		throw std::invalid_argument("Invalid port number");

	if (host.empty())
		throw std::invalid_argument("Host cannot be empty");

	if (root.empty())
		throw std::invalid_argument("Root cannot be empty");

	if (index.empty())
		throw std::invalid_argument("Index cannot be empty");

}

Webserv::Webserv() : port(80), host("localhost"), root("/var/www/html"), serverName("default_server"), index("index.html") {
	// Default constructor initializes with some default values
	this->locations["/"] = this->root;
	this->errorPageMap[404] = "404.html";
	this->errorPageMap[500] = "500.html";
}

Webserv::Webserv(const Config& config) {
	this->port = config.getPort();
	this->host = config.getHost();
	this->root = config.getRoot();
	this->serverName = config.getServerName();
	this->index = config.getIndex();
	this->locations = config.getLocations();
	this->errorPageMap = config.getErrorPageMap();
	try {
		this->checkAttributes();
	} catch (const std::invalid_argument& e) {
		std::cerr << "Error in Webserv configuration: " << e.what() << std::endl;
		throw;
	}

}

Webserv::~Webserv() {}

