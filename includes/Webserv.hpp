#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "origin.hpp"
#include "Config.hpp"

class Webserv {
	private:
		int			port;
		std::string	host;
		std::string	root;
		std::string	serverName;
		std::string	index;
		std::map<std::string, std::string> locations;
		std::map<int, std::string> errorPageMap;
		typedef struct	cgi
		{
			std::string extension;
			std::string path;
		}				cgi_t;
		
		std::vector<cgi_t> cgiList;
		
	public:
		Webserv();
		Webserv(const Config& config);
		~Webserv();
		void checkAttributes() const;
};

#endif