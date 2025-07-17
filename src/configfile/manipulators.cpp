#include "../../includes/Config.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/identifiers.hpp"

void removeInvalidChars(std::string &str) {
	if (str.empty())
		return ;
	str.erase(std::remove_if(str.begin(), str.end(), notValidChar), str.end());
}

std::string	saveArray(const std::string& line) {
	std::vector<std::string> array;
	if (std::count(line.begin(), line.end(), '"') != (std::count(line.begin(), line.end(), ',') + 1) * 2)
		throw std::runtime_error("Invalid array format: " + line);
	size_t start = 0;
	size_t end = line.find(',');
	std::string value;
	
	while (end != NPOS) {
		value = line.substr(start, end - start);
		removeInvalidChars(value);
		if (value.empty()) {
			throw std::runtime_error("Invalid array value: " + value);
		}
		array.push_back(value);
		value.clear();
		start = end + 1;
		end = line.find_first_of(",]", start);
	}
	std::string merged;
	for (size_t i = 0; i < array.size(); ++i) {
		merged += array[i] + (i < array.size() - 1 ? "," : "");
	}
	return merged;
}

