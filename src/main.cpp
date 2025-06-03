#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return 1;
	}

	std::string configFile = argv[1];
	Config server1(configFile);
	return 0;
}