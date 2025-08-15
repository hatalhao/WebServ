#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/request.hpp"

# define BRIGHT_BLUE "\x1b[94m"
# define BRIGHT_YELLOW "\x1b[93m"

int main(int ac, char **av) {
    
    if (ac != 2)
        return (cerr << "invalid args!" << endl, 1);
    try {
        Config	webservConfig(string("config/file.toml"));
        Client obj(-1, &webservConfig._servers[0]);
        fstream	raw(av[1]);
        string line;
        
        while (getline(raw, line)) {
            obj.setRequestBuf(obj.getRequestBuf() + line + "\n");
            obj.processRequest();
        }
        raw.close(); 
        cout << BRIGHT_GREEN "parsing complete" RESET << endl;
        obj.generateResponse();
        // obj.printRequest();
        // obj.printState();
        // obj.printBuffer();
        } catch (exception& e) {
            cout << e.what() << endl;
        }
    return 0;
}
