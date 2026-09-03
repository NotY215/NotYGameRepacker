#pragma once
#include <string>
#include <vector>

namespace noty {
	namespace utils {

		std::string formatFileSize(uint64_t bytes);
		std::string formatDuration(uint64_t seconds);
		std::vector<std::string> splitString(const std::string& str, char delimiter);
		bool isValidFilename(const std::string& filename);

	} // namespace utils
} // namespace noty