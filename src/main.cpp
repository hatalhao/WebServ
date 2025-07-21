#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/request.hpp"

# define BRIGHT_BLUE "\x1b[94m"
# define BRIGHT_YELLOW "\x1b[93m"

int main(int ac, char **av) {
    
    if (ac != 2)
        return (cerr << "invalid args!" << endl, 1);
    Config	webservConfig(string("config/file.toml"));
    cout << BRIGHT_GREEN "===============" RESET << endl;
    fstream	raw(av[1]);
    string line;
    Client obj(-1, &webservConfig);
    
    while (getline(raw, line)) {
        obj.setRequestBuf(obj.getRequestBuf() + line + "\n");
        // cout << BRIGHT_BLUE "Processing line: " RESET << line << endl;
        // cout << BRIGHT_YELLOW "Current buffer length: " RESET << obj.getRequestBuf().length() << endl;
        obj.processRequest();
        // obj.printState();
        // cout << "---" << endl;
    }
    raw.close(); 

    cout << BRIGHT_GREEN "Final processing complete" RESET << endl;
	obj.printState();
    obj.printRequest();
    obj.printBuffer();
    return 0;
}
