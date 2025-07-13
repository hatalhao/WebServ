#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "origin.hpp"
struct server;

enum tokens {
	server_name = 0,
	host,
	listen,
	root,
	index,
	path,
	name,
	autoindex,
	allow_methods,
	upload,
	redirection,
	upload_path,
	upload_max_size,
	e300,
	e301,
	e302,
	e303,
	e304,
	e305,
	e306,
	e307,
	e308,
	e400,
	e403,
	e404,
	e408,	
	e405,
	e500,
	e501,
	e502,
	e503,
	e504,
	e505,
};

class Config {
	private:
		std::string _configFilePath;
		std::map<std::string, std::string> _settings;
		std::vector<server> _servers;

	public:
		Config(const std::string &configFilePath);
		~Config();

		//extracting methods
		std::set<int>						extractPorts() const;
		std::string							extractHost() const;
		std::string							extractRoot() const;
		std::vector<std::string>			extractIndex() const;
		std::vector<std::string>			extractServerName() const;
		std::map<std::string, std::string>	extractLocations() const;
		std::map<int, std::string>			extractErrorPageMap() const;
	

		void printParams() {
			std::map<std::string, std::string>::iterator it = _settings.begin();
			std::map<std::string, std::string>::iterator it1 = _settings.end();
			std::cout << "Configuration file path: " << _configFilePath << std::endl;
			std::cout << "Configuration settings:" << std::endl;
			std::pair<std::string, std::string> _pair;
			while (it != it1) {
				_pair = *it;
				std::cout << _pair.first << "=" << _pair.second << std::endl;
				std::cout << "------------------------" << std::endl;
				it++;
			}
	}

}; 

#endif