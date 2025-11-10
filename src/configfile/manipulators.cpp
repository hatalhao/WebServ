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
			size = std::atoll(numberStr.c_str()) * GB;
		} else {
			throw std::runtime_error("Invalid size unit: " + unitStr);
		}
	}
	else {
		size = std::atoll(sizeStr.c_str());
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

static inline bool validSeparators(const std::string& line) {
	int count = 0;
	for (size_t i = 0; i < line.size(); i++) {
		if (line[i] == '\"')
			count++;
		if (count == 2 && (line[i + 1] && line[i + 1] != ',' && line[i + 1] != ' ') && i != line.size()) {
			return false;
		}
		if (line[i] == ',')
			count = 0;
		if (count > 2)
			return false;
	}
	return true;
}

static inline bool checkForSingle(const std::string& line, size_t start, size_t end) {
	size_t doubleQuoteCount = std::count(line.begin() + start, line.begin() + end + 1, '\"');
	size_t singleQuoteCount = std::count(line.begin() + start, line.begin() + end + 1, '\'');
	if (doubleQuoteCount == 2 && singleQuoteCount == 0 && line[start] == '\"' && line[end] == '\"')
		return (true);
	if (singleQuoteCount == 2 && doubleQuoteCount == 0 && line[start] == '\'' && line[end] == '\'')
		return (true);
	return (false);
}

static inline bool validArray(const std::string& line, size_t start, size_t end) {
	size_t beginBracket = line.find('[', start);
	size_t endBracket = line.find(']', beginBracket);

	if (beginBracket != NPOS && endBracket != NPOS && beginBracket == start && endBracket == end)
		return true;
	if (checkForSingle(line, start, end))
		return (true);
	return (false);
}

static inline std::string getEntries(const std::string& line) {
	size_t start = line.find_first_not_of(" \t");
	size_t end = line.find_last_not_of(" \t");
	if (start == NPOS || end == NPOS || end <= start || !validArray(line, start, end))
		throw std::runtime_error("Invalid array format: " + line);
	std::string  entries = line.substr(start + (line[start] == '['), end - start + 1 - (line[end] == ']') * 2);
	if (!validSeparators(entries))
		throw std::runtime_error("Invalid array separators: " + entries);
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
	size_t start, end;
	while (std::getline(ss, value, ',')) {
		if (value.find_first_not_of(" \t") == NPOS || value.empty()) {
			throw std::runtime_error("Empty value in array: " + line);
		}
		start = 0, end = value.size() - 1;
		start = value.find_first_not_of(" \t");
		end = value.find_last_not_of(" \t");
		value = value.substr(start, end - start + 1);
		merged += value + (ss.eof() ? "" : ",");
	}
	
	return merged;
}