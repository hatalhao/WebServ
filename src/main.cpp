#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/worker.hpp"
#include <csignal>

# define q "\x1b[94m"
# define BRIGHT_YELLOW "\x1b[93m"

void    signalHandler(int signum) {
    Worker::cleanup();
    exit(signum);
}

int main(int ac, char **av) {
    if (ac > 2) {
        std::cerr << BRIGHT_YELLOW "Usage: " << av[0] << " [config_file]" RESET << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    try {
        Worker* master = Worker::getInstance(ac == 2 ? av[1] : "config/default.toml");
		master->establishConnection();
        } catch (exception& e) {
            cout << e.what() << endl;
        }
    return 0;
}
