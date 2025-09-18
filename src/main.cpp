#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/worker.hpp"
#include <csignal>

# define BRIGHT_BLUE "\x1b[94m"
# define BRIGHT_YELLOW "\x1b[93m"

void    signalHandler(int signum) {
    std::cerr << BRIGHT_YELLOW "Signal " << signum << " received. Cleaning up..." RESET << std::endl;
    Worker::cleanup();
    exit(signum);
}

int main(int ac, char **av) {
    // std::string filename;
    // else if (ac == 2)
    //     filename = av[1];
    // else {
    //     std::cerr << BRIGHT_YELLOW "Usage: " << av[0] << " [config_file]" RESET << std::endl;
    //     return 1;
    // }
    signal(SIGINT, signalHandler);
    try {
        Worker* master = Worker::getInstance(ac == 2 ? av[1] : "config/default.toml");
		master->establishConnection();
        } catch (exception& e) {
            cout << e.what() << endl;
        }
    return 0;
}
