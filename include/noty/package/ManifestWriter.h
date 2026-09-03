#pragma once
#include "Manifest.h"
#include <string>
#include <vector>

namespace noty {

    class ManifestWriter {
    public:
        ManifestWriter() = default;
        ~ManifestWriter() = default;

        // Write manifest to JSON string
        std::string writeToString(const Manifest& manifest) const;

        // Write manifest to file
        bool writeToFile(const Manifest& manifest, const std::string& filename) const;

        // Write manifest to file with pretty formatting
        bool writeToFilePretty(const Manifest& manifest, const std::string& filename) const;

        // Write only package info (without files list)
        std::string writePackageInfo(const PackageInfo& info) const;

        // Convert FileEntry to JSON string
        std::string fileEntryToJson(const FileEntry& file) const;

    private:
        std::string escapeJson(const std::string& str) const;
    };

} // namespace noty