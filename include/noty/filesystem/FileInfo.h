#pragma once
#include <string>
#include <cstdint>
#include <chrono>

namespace noty {

    struct FileInfo {
        std::string path;           // Full path
        std::string relativePath;   // Relative to source root
        std::string filename;       // Just the filename
        uint64_t size = 0;          // Size in bytes
        uint64_t lastModified = 0;  // Last modified timestamp
        bool isDirectory = false;
        bool isHidden = false;

        // For future use - file hash will be added later
        // std::string hash;

        FileInfo() = default;
        FileInfo(const std::string& path, uint64_t size, bool isDir);

        std::string getExtension() const;
        bool isExecutable() const;
        bool isGameExecutable() const;
    };

} // namespace noty