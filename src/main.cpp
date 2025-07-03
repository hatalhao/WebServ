#include "../includes/Config.hpp"
#include "../includes/Webserv.hpp"

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return 1;
	}

	std::string configFile = argv[1];
	Config server1(configFile);
	server1.printParams();
	return 0;
}