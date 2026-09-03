#pragma once
#include "Manifest.h"
#include <string>
#include <optional>

namespace noty {

    class ManifestReader {
    public:
        ManifestReader() = default;
        ~ManifestReader() = default;

        // Read manifest from JSON string
        std::optional<Manifest> readFromString(const std::string& jsonStr) const;

        // Read manifest from file
        std::optional<Manifest> readFromFile(const std::string& filename) const;

        // Read only package info from JSON
        std::optional<PackageInfo> readPackageInfo(const std::string& jsonStr) const;

        // Validate manifest file without fully parsing
        bool validateManifestFile(const std::string& filename) const;

    private:
        struct ParseContext {
            Manifest manifest;
            std::vector<std::string> errors;
        };

        bool parseFileEntry(const nlohmann::json& j, FileEntry& entry) const;
        bool parseChunkInfo(const nlohmann::json& j, ChunkInfo& chunk) const;
        bool parseComponentInfo(const nlohmann::json& j, ComponentInfo& component) const;
        bool parsePackageInfo(const nlohmann::json& j, PackageInfo& info) const;

        void addError(ParseContext& ctx, const std::string& error) const;
    };

} // namespace noty