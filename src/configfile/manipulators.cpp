#include "../../includes/Config.hpp"
#include "../../includes/Webserv.hpp"
#include "../../includes/identifiers.hpp"
#include "../../includes/manipulators.hpp"

size_t parseSize(const std::string& sizeStr) {
	if (sizeStr.empty()) {
		throw std::runtime_error("Size string is empty");
	}
	if (sizeStr[0] == '-') {
		throw std::runtime_error("Size cannot be negative: " + sizeStr);
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
	size_t start = str.find_first_not_of(" \t\"\'");
	size_t end = str.find_last_not_of(" \t\"\'");
	if (start == NPOS || end == NPOS) {
		str.clear();
	} else {
		str = str.substr(start, end - start + 1);
	}
}

static inline std::string getEntries(const std::string& line) {
	size_t start = line.find_first_not_of(" \t[");
	size_t end = line.find_last_not_of(" \t]");
	if (start == NPOS || end == NPOS || end <= start)
		return ("");
	std::string  entries = line.substr(start, end - start + 1);
	entries.erase(std::remove(entries.begin(), entries.end(), '"'), entries.end());
	return (entries);
}

std::string	saveArray(const std::string& line) {
	std::vector<std::string> array;
	if (std::count(line.begin(), line.end(), '"') != (std::count(line.begin(), line.end(), ',') + 1) * 2)
		throw std::runtime_error("Invalid array format: " + line);

	std::string value = getEntries(line);
	std::stringstream ss(value);
	value.clear();
	std::string merged;
	while (std::getline(ss, value, ',')) {
		if (value.empty()) {
			throw std::runtime_error("Empty value in array: " + line);
		}
		merged += value + ",";
	}

	return merged;
}