#include "noty/package/ManifestWriter.h"
#include "noty/common/Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <ctime>

using json = nlohmann::json;

namespace noty {

    std::string ManifestWriter::escapeJson(const std::string& str) const {
        // nlohmann::json handles escaping automatically
        // This is kept for potential future use
        return str;
    }

    std::string ManifestWriter::fileEntryToJson(const FileEntry& file) const {
        json j;
        j["path"] = file.path;
        j["size"] = file.size;
        j["hash"] = file.hash;
        j["compressedSize"] = file.compressedSize;
        j["chunkId"] = file.chunkId;
        j["offsetInChunk"] = file.offsetInChunk;
        j["isOptional"] = file.isOptional;
        if (!file.component.empty()) {
            j["component"] = file.component;
        }
        return j.dump();
    }

    std::string ManifestWriter::writeToString(const Manifest& manifest) const {
        json j;

        // Package info
        const auto& info = manifest.getPackageInfo();
        j["packageInfo"]["packageId"] = info.packageId;
        j["packageInfo"]["gameName"] = info.gameName;
        j["packageInfo"]["gameVersion"] = info.gameVersion;
        j["packageInfo"]["repackerName"] = info.repackerName;
        j["packageInfo"]["setupName"] = info.setupName;
        j["packageInfo"]["coverFormat"] = info.coverFormat;
        j["packageInfo"]["coverSize"] = info.coverSize;
        j["packageInfo"]["originalSize"] = info.originalSize;
        j["packageInfo"]["compressedSize"] = info.compressedSize;
        j["packageInfo"]["compressionRatio"] = info.compressionRatio;
        j["packageInfo"]["chunkCount"] = info.chunkCount;
        j["packageInfo"]["fileCount"] = info.fileCount;
        j["packageInfo"]["compressionMethod"] = info.compressionMethod;
        j["packageInfo"]["encryptionMethod"] = info.encryptionMethod;
        j["packageInfo"]["hashAlgorithm"] = info.hashAlgorithm;
        j["packageInfo"]["formatVersion"] = info.formatVersion;
        j["packageInfo"]["createdBy"] = info.createdBy;

        // Creation date
        auto time = std::chrono::system_clock::to_time_t(info.creationDate);
        j["packageInfo"]["creationDate"] = std::ctime(&time);
        // Remove trailing newline
        std::string dateStr = j["packageInfo"]["creationDate"];
        if (!dateStr.empty() && dateStr.back() == '\n') {
            dateStr.pop_back();
        }
        j["packageInfo"]["creationDate"] = dateStr;

        // Files
        json filesArray = json::array();
        for (const auto& file : manifest.getFiles()) {
            json fileJson;
            fileJson["path"] = file.path;
            fileJson["size"] = file.size;
            fileJson["hash"] = file.hash;
            fileJson["compressedSize"] = file.compressedSize;
            fileJson["chunkId"] = file.chunkId;
            fileJson["offsetInChunk"] = file.offsetInChunk;
            fileJson["isOptional"] = file.isOptional;
            if (!file.component.empty()) {
                fileJson["component"] = file.component;
            }
            filesArray.push_back(fileJson);
        }
        j["files"] = filesArray;

        // Chunks
        json chunksArray = json::array();
        for (const auto& chunk : manifest.getChunks()) {
            json chunkJson;
            chunkJson["id"] = chunk.id;
            chunkJson["filename"] = chunk.filename;
            chunkJson["compressedSize"] = chunk.compressedSize;
            chunkJson["uncompressedSize"] = chunk.uncompressedSize;
            chunkJson["fileCount"] = chunk.fileCount;
            chunkJson["checksum"] = chunk.checksum;
            chunksArray.push_back(chunkJson);
        }
        j["chunks"] = chunksArray;

        // Components
        json componentsArray = json::array();
        for (const auto& component : manifest.getComponents()) {
            json compJson;
            compJson["name"] = component.name;
            compJson["description"] = component.description;
            compJson["size"] = component.size;
            compJson["isRequired"] = component.isRequired;
            compJson["filePatterns"] = component.filePatterns;
            componentsArray.push_back(compJson);
        }
        j["components"] = componentsArray;

        return j.dump(2); // Pretty print with 2 spaces
    }

    std::string ManifestWriter::writePackageInfo(const PackageInfo& info) const {
        json j;
        j["packageId"] = info.packageId;
        j["gameName"] = info.gameName;
        j["gameVersion"] = info.gameVersion;
        j["repackerName"] = info.repackerName;
        j["setupName"] = info.setupName;
        j["coverFormat"] = info.coverFormat;
        j["coverSize"] = info.coverSize;
        j["originalSize"] = info.originalSize;
        j["compressedSize"] = info.compressedSize;
        j["compressionRatio"] = info.compressionRatio;
        j["chunkCount"] = info.chunkCount;
        j["fileCount"] = info.fileCount;
        j["compressionMethod"] = info.compressionMethod;
        j["encryptionMethod"] = info.encryptionMethod;
        j["hashAlgorithm"] = info.hashAlgorithm;
        j["formatVersion"] = info.formatVersion;
        j["createdBy"] = info.createdBy;

        auto time = std::chrono::system_clock::to_time_t(info.creationDate);
        j["creationDate"] = std::ctime(&time);
        std::string dateStr = j["creationDate"];
        if (!dateStr.empty() && dateStr.back() == '\n') {
            dateStr.pop_back();
        }
        j["creationDate"] = dateStr;

        return j.dump(2);
    }

    bool ManifestWriter::writeToFile(const Manifest& manifest, const std::string& filename) const {
        try {
            std::string jsonStr = writeToString(manifest);
            std::ofstream file(filename);
            if (!file.is_open()) {
                Logger::instance().error("Failed to open file for writing: " + filename);
                return false;
            }
            file << jsonStr;
            file.close();

            Logger::instance().info("Manifest written to: " + filename);
            return true;
        }
        catch (const std::exception& e) {
            Logger::instance().error("Failed to write manifest: " + std::string(e.what()));
            return false;
        }
    }

    bool ManifestWriter::writeToFilePretty(const Manifest& manifest, const std::string& filename) const {
        // Same as writeToFile since we already use pretty formatting
        return writeToFile(manifest, filename);
    }

} // namespace noty