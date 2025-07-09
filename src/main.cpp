#include "../includes/Config.hpp"
#include "../includes/Webserv.hpp"

int main(int argc, char **argv) {
	if (argc < 1 || argc > 2) {
		std::cerr << "Usage: ./webserv <config_file>" << std::endl;
		return 1;
	}
	std::string configFile = (argv[1]) ? argv[1] : "config/default.toml";
	try {
		Config conf(configFile);
	} catch (const std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
