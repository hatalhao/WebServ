#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"

int main(int argc, char **argv) {
	if (argc < 1 || argc > 2) {
		std::cerr << "Usage: ./" << argv[0] << " <config_file>" << std::endl;
		return 1;
	}
	std::string configFile = (argv[1]) ? argv[1] : "config/default.toml";
	std::map<std::string, std::string> settings;
	settings.clear();
	
	try {
		Config conf(configFile);
		// server serv;
		// fillServer(serv, conf);
		try {
			checkAttributes(conf._servers[1]);
		} catch (const std::invalid_argument &e) {
			std::cerr << "Invalid server configuration: " << e.what() << std::endl;
			return 1;
		}
	} catch (const std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
