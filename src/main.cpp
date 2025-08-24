#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/request.hpp"

# define BRIGHT_BLUE "\x1b[94m"
# define BRIGHT_YELLOW "\x1b[93m"

int main(int ac, char **av) {
    std::string filename;
    if (ac == 1)
        filename = "config/default.toml";
    else if (ac == 2)
        filename = av[1];
    else {
        std::cerr << BRIGHT_YELLOW "Usage: " << av[0] << " [config_file]" RESET << std::endl;
        return 1;
    }

    try {
        Config	webservConfig(filename);
        initialiseServer(webservConfig);
        } catch (exception& e) {
            cout << e.what() << endl;
        }
    return 0;
}
