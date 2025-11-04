#include "../../includes/Config.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"

size_t parseSize(const std::string& sizeStr) {
	if (sizeStr.empty()) {
		throw std::runtime_error("Size string is empty");
	}
	size_t size = 0;
	size_t pos = sizeStr.find_first_not_of("0123456789");
	std::string numberStr = sizeStr.substr(0, pos);
	if (pos != NPOS) {
		std::string unitStr = sizeStr.substr(pos);
		std::transform(unitStr.begin(), unitStr.end(), unitStr.begin(), ::toupper);
		if (unitStr == "B") {
			size = std::atoll(numberStr.c_str());
		} else if (unitStr == "KB") {
			size = std::atoll(numberStr.c_str()) * KB;
		} else if (unitStr == "MB") {
			size = std::atoll(numberStr.c_str()) * MB;
		} else if (unitStr == "GB") {
			throw std::runtime_error("Size unit GB is not supported");
		} else {
			throw std::runtime_error("Invalid size unit: " + unitStr);
		}
	} else {
		size = std::atoll(sizeStr.c_str());
	}
	if (size > 50 * MB) {
		throw std::runtime_error("Size exceeds maximum allowed limit of 50MB");
	}
	return size;
}

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
	if (end == NPOS && start == 0){
		value = line;
		removeInvalidChars(value);
		if (value.empty()) {
			throw std::runtime_error("Invalid array value: " + value);
		}
		merged = value;
	}
	for (size_t i = 0; i < array.size(); ++i) {
		merged += array[i] + (i < array.size() - 1 ? "," : "");
	}
	return merged;
}