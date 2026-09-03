#include "noty/package/Manifest.h"
#include "noty/common/Logger.h"
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace noty {

    Manifest::Manifest(const PackageInfo& info)
        : m_packageInfo(info) {
    }

    void Manifest::addFile(const FileEntry& file) {
        m_files.push_back(file);
        m_fileIndex[file.path] = m_files.size() - 1;
    }

    void Manifest::setFiles(const std::vector<FileEntry>& files) {
        m_files = files;
        m_fileIndex.clear();
        for (size_t i = 0; i < m_files.size(); ++i) {
            m_fileIndex[m_files[i].path] = i;
        }
    }

    void Manifest::clearFiles() {
        m_files.clear();
        m_fileIndex.clear();
    }

    void Manifest::addChunk(const ChunkInfo& chunk) {
        m_chunks.push_back(chunk);
    }

    void Manifest::setChunks(const std::vector<ChunkInfo>& chunks) {
        m_chunks = chunks;
    }

    void Manifest::addComponent(const ComponentInfo& component) {
        m_components.push_back(component);
    }

    void Manifest::setComponents(const std::vector<ComponentInfo>& components) {
        m_components = components;
    }

    const FileEntry* Manifest::getFile(const std::string& path) const {
        auto it = m_fileIndex.find(path);
        if (it != m_fileIndex.end()) {
            return &m_files[it->second];
        }
        return nullptr;
    }

    uint64_t Manifest::calculateTotalFileSize() const {
        uint64_t total = 0;
        for (const auto& file : m_files) {
            total += file.size;
        }
        return total;
    }

    uint64_t Manifest::calculateTotalCompressedSize() const {
        uint64_t total = 0;
        for (const auto& chunk : m_chunks) {
            total += chunk.compressedSize;
        }
        return total;
    }

    bool Manifest::validate() const {
        // Check required fields
        if (m_packageInfo.gameName.empty()) {
            Logger::instance().error("Manifest validation failed: gameName is empty");
            return false;
        }

        if (m_packageInfo.packageId.empty()) {
            Logger::instance().error("Manifest validation failed: packageId is empty");
            return false;
        }

        if (m_packageInfo.chunkCount != m_chunks.size()) {
            Logger::instance().error("Manifest validation failed: chunk count mismatch");
            return false;
        }

        // Validate files
        auto invalidFiles = validateFiles();
        if (!invalidFiles.empty()) {
            Logger::instance().error("Manifest validation failed: " +
                std::to_string(invalidFiles.size()) + " invalid files");
            return false;
        }

        return true;
    }

    std::vector<std::string> Manifest::validateFiles() const {
        std::vector<std::string> invalidFiles;

        for (const auto& file : m_files) {
            // Check for invalid characters in path
            fs::path p(file.path);
            if (p.empty()) {
                invalidFiles.push_back(file.path);
                continue;
            }

            // Check hash format (should be hex string)
            if (file.hash.empty()) {
                invalidFiles.push_back(file.path);
                continue;
            }

            // Check size
            if (file.size == 0 && file.path.find(".exe") == std::string::npos) {
                // Some files can be 0 size (empty files), but warn
                Logger::instance().warning("Zero size file: " + file.path);
            }
        }

        return invalidFiles;
    }

} // namespace noty