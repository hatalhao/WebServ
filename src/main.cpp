#include "../includes/Webserv.hpp"
#include "../includes/Config.hpp"
#include "../includes/request.hpp"

int main() {
	Client obj(-1);
	obj.setRequestBuf("GET /index.html HTTP/1.1\r\nHost: localhost\nContent-len: 334\r\n\r\n");
	obj.processRequest();
	obj.processRequest();
	obj.printRequest();
	return 0;
}
