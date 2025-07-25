#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/request.hpp"

# define BRIGHT_BLUE "\x1b[94m"
# define BRIGHT_YELLOW "\x1b[93m"

int main(int ac, char **av) {
	
	if (ac != 2)
		return (cerr << "invalid args!" << endl, 1);
	try {
		Config	webservConfig(av[1]);
		initialiseServer(webservConfig);
		} catch (exception& e) {
			cout << e.what() << endl;
		}
	return 0;
}
