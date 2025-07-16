#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/request.hpp"

int main(int ac, char **av) {
	
	if (ac != 2)
		return (cerr << "invalid args!" << endl, 1);
	fstream	raw(av[1]);
	string line;
	string fileContent;
	
	while (getline(raw, line)) {
		fileContent += line + "\n";
	}
	raw.close(); 
	Client obj(-1);
	obj.setRequestBuf(fileContent);
	obj.processRequest();
	obj.printState();
	obj.processRequest();
	obj.printRequest();
	obj.printState();
	obj.printBuffer();
	return 0;
}
