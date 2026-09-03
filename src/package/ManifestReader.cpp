#include "noty/package/ManifestReader.h"
#include "noty/common/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <chrono>

using json = nlohmann::json;

namespace noty {

    bool ManifestReader::parseFileEntry(const json& j, FileEntry& entry) const {
        try {
            entry.path = j.value("path", "");
            entry.size = j.value("size", 0ULL);
            entry.hash = j.value("hash", "");
            entry.compressedSize = j.value("compressedSize", 0ULL);
            entry.chunkId = j.value("chunkId", 0U);
            entry.offsetInChunk = j.value("offsetInChunk", 0ULL);
            entry.isOptional = j.value("isOptional", false);
            entry.component = j.value("component", "");

            if (entry.path.empty()) {
                return false;
            }
            return true;
        }
        catch (const std::exception& e) {
            return false;
        }
    }

    bool ManifestReader::parseChunkInfo(const json& j, ChunkInfo& chunk) const {
        try {
            chunk.id = j.value("id", 0U);
            chunk.filename = j.value("filename", "");
            chunk.compressedSize = j.value("compressedSize", 0ULL);
            chunk.uncompressedSize = j.value("uncompressedSize", 0ULL);
            chunk.fileCount = j.value("fileCount", 0U);
            chunk.checksum = j.value("checksum", "");

            return !chunk.filename.empty();
        }
        catch (const std::exception& e) {
            return false;
        }
    }

    bool ManifestReader::parseComponentInfo(const json& j, ComponentInfo& component) const {
        try {
            component.name = j.value("name", "");
            component.description = j.value("description", "");
            component.size = j.value("size", 0ULL);
            component.isRequired = j.value("isRequired", false);

            if (j.contains("filePatterns") && j["filePatterns"].is_array()) {
                for (const auto& pattern : j["filePatterns"]) {
                    component.filePatterns.push_back(pattern.get<std::string>());
                }
            }

            return !component.name.empty();
        }
        catch (const std::exception& e) {
            return false;
        }
    }

    bool ManifestReader::parsePackageInfo(const json& j, PackageInfo& info) const {
        try {
            info.packageId = j.value("packageId", "");
            info.gameName = j.value("gameName", "");
            info.gameVersion = j.value("gameVersion", "");
            info.repackerName = j.value("repackerName", "");
            info.setupName = j.value("setupName", "");
            info.coverFormat = j.value("coverFormat", "");
            info.coverSize = j.value("coverSize", 0ULL);
            info.originalSize = j.value("originalSize", 0ULL);
            info.compressedSize = j.value("compressedSize", 0ULL);
            info.compressionRatio = j.value("compressionRatio", 0ULL);
            info.chunkCount = j.value("chunkCount", 0U);
            info.fileCount = j.value("fileCount", 0U);
            info.compressionMethod = j.value("compressionMethod", "");
            info.encryptionMethod = j.value("encryptionMethod", "");
            info.hashAlgorithm = j.value("hashAlgorithm", "");
            info.formatVersion = j.value("formatVersion", 1);
            info.createdBy = j.value("createdBy", "");

            // Parse creation date
            if (j.contains("creationDate")) {
                std::string dateStr = j["creationDate"];
                // Try to parse date (simple approach)
                // For production, use proper date parsing
                info.creationDate = std::chrono::system_clock::now();
            }

            return !info.packageId.empty() && !info.gameName.empty();
        }
        catch (const std::exception& e) {
            return false;
        }
    }

    void ManifestReader::addError(ParseContext& ctx, const std::string& error) const {
        ctx.errors.push_back(error);
        Logger::instance().error("Manifest parse error: " + error);
    }

    std::optional<Manifest> ManifestReader::readFromString(const std::string& jsonStr) const {
        ParseContext ctx;

        try {
            json j = json::parse(jsonStr);

            // Parse package info
            if (j.contains("packageInfo")) {
                if (!parsePackageInfo(j["packageInfo"], ctx.manifest.m_packageInfo)) {
                    addError(ctx, "Failed to parse package info");
                }
            }
            else {
                addError(ctx, "Missing packageInfo section");
            }

            // Parse files
            if (j.contains("files") && j["files"].is_array()) {
                for (const auto& fileJson : j["files"]) {
                    FileEntry entry;
                    if (parseFileEntry(fileJson, entry)) {
                        ctx.manifest.addFile(entry);
                    }
                    else {
                        addError(ctx, "Failed to parse file entry");
                    }
                }
            }

            // Parse chunks
            if (j.contains("chunks") && j["chunks"].is_array()) {
                for (const auto& chunkJson : j["chunks"]) {
                    ChunkInfo chunk;
                    if (parseChunkInfo(chunkJson, chunk)) {
                        ctx.manifest.addChunk(chunk);
                    }
                    else {
                        addError(ctx, "Failed to parse chunk entry");
                    }
                }
            }

            // Parse components
            if (j.contains("components") && j["components"].is_array()) {
                for (const auto& compJson : j["components"]) {
                    ComponentInfo comp;
                    if (parseComponentInfo(compJson, comp)) {
                        ctx.manifest.addComponent(comp);
                    }
                    else {
                        addError(ctx, "Failed to parse component entry");
                    }
                }
            }

            if (!ctx.errors.empty()) {
                return std::nullopt;
            }

            return ctx.manifest;

        }
        catch (const std::exception& e) {
            Logger::instance().error("Failed to parse manifest JSON: " + std::string(e.what()));
            return std::nullopt;
        }
    }

    std::optional<Manifest> ManifestReader::readFromFile(const std::string& filename) const {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                Logger::instance().error("Failed to open manifest file: " + filename);
                return std::nullopt;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();

            return readFromString(buffer.str());

        }
        catch (const std::exception& e) {
            Logger::instance().error("Failed to read manifest file: " + std::string(e.what()));
            return std::nullopt;
        }
    }

    std::optional<PackageInfo> ManifestReader::readPackageInfo(const std::string& jsonStr) const {
        try {
            json j = json::parse(jsonStr);
            PackageInfo info;
            if (parsePackageInfo(j, info)) {
                return info;
            }
            return std::nullopt;
        }
        catch (const std::exception& e) {
            Logger::instance().error("Failed to parse package info: " + std::string(e.what()));
            return std::nullopt;
        }
    }

    bool ManifestReader::validateManifestFile(const std::string& filename) const {
        auto manifest = readFromFile(filename);
        if (!manifest.has_value()) {
            return false;
        }
        return manifest->validate();
    }

} // namespace noty