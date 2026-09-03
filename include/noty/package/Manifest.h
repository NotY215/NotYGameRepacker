#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <chrono>

namespace noty {

    struct FileEntry {
        std::string path;
        uint64_t size = 0;
        std::string hash;
        uint64_t compressedSize = 0;
        uint32_t chunkId = 0;
        uint64_t offsetInChunk = 0;
        bool isOptional = false;
        std::string component;

        FileEntry() = default;
        FileEntry(const std::string& p, uint64_t s, const std::string& h)
            : path(p), size(s), hash(h) {
        }
    };

    struct ChunkInfo {
        uint32_t id = 0;
        std::string filename;
        uint64_t compressedSize = 0;
        uint64_t uncompressedSize = 0;
        uint32_t fileCount = 0;
        std::string checksum;
    };

    struct ComponentInfo {
        std::string name;
        std::string description;
        uint64_t size = 0;
        bool isRequired = false;
        std::vector<std::string> filePatterns;
    };

    struct PackageInfo {
        std::string packageId;
        std::string gameName;
        std::string gameVersion;
        std::string repackerName;
        std::string setupName;
        std::string coverFormat;
        uint64_t coverSize = 0;
        uint64_t originalSize = 0;
        uint64_t compressedSize = 0;
        uint64_t compressionRatio = 0;
        uint32_t chunkCount = 0;
        uint32_t fileCount = 0;
        std::string compressionMethod;
        std::string encryptionMethod;
        std::string hashAlgorithm;
        uint32_t formatVersion = 1;
        std::chrono::system_clock::time_point creationDate;
        std::string createdBy;
    };

    class Manifest {
    public:
        Manifest() = default;
        explicit Manifest(const PackageInfo& info);

        // Package info - public for access by readers/writers
        PackageInfo m_packageInfo;

        const PackageInfo& getPackageInfo() const { return m_packageInfo; }
        void setPackageInfo(const PackageInfo& info) { m_packageInfo = info; }

        const std::vector<FileEntry>& getFiles() const { return m_files; }
        void addFile(const FileEntry& file);
        void setFiles(const std::vector<FileEntry>& files);
        void clearFiles();

        const std::vector<ChunkInfo>& getChunks() const { return m_chunks; }
        void addChunk(const ChunkInfo& chunk);
        void setChunks(const std::vector<ChunkInfo>& chunks);

        const std::vector<ComponentInfo>& getComponents() const { return m_components; }
        void addComponent(const ComponentInfo& component);
        void setComponents(const std::vector<ComponentInfo>& components);

        const FileEntry* getFile(const std::string& path) const;

        uint64_t calculateTotalFileSize() const;
        uint64_t calculateTotalCompressedSize() const;

        bool validate() const;
        std::vector<std::string> validateFiles() const;

    private:
        std::vector<FileEntry> m_files;
        std::vector<ChunkInfo> m_chunks;
        std::vector<ComponentInfo> m_components;
        std::unordered_map<std::string, size_t> m_fileIndex;
    };

} // namespace noty