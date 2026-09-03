#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <chrono>

namespace noty {

    struct FileEntry {
        std::string path;           // Relative path from game root
        uint64_t size = 0;          // Original file size in bytes
        std::string hash;           // BLAKE3 or SHA-256 hash (hex string)
        uint64_t compressedSize = 0; // Size after compression
        uint32_t chunkId = 0;       // Which chunk this file belongs to
        uint64_t offsetInChunk = 0; // Offset within the chunk
        bool isOptional = false;    // For optional components
        std::string component;      // Component name if optional

        FileEntry() = default;
        FileEntry(const std::string& p, uint64_t s, const std::string& h)
            : path(p), size(s), hash(h) {
        }
    };

    struct ChunkInfo {
        uint32_t id = 0;
        std::string filename;       // e.g., "GameName.001.noty"
        uint64_t compressedSize = 0;
        uint64_t uncompressedSize = 0;
        uint32_t fileCount = 0;
        std::string checksum;       // Integrity check for the chunk
    };

    struct ComponentInfo {
        std::string name;
        std::string description;
        uint64_t size = 0;
        bool isRequired = false;
        std::vector<std::string> filePatterns; // Patterns for files in this component
    };

    struct PackageInfo {
        std::string packageId;          // Unique ID
        std::string gameName;
        std::string gameVersion;
        std::string repackerName;       // "Repacked by %USERNAME%"
        std::string setupName;          // Setup.exe filename

        std::string coverFormat;        // png, jpg, etc.
        uint64_t coverSize = 0;

        uint64_t originalSize = 0;
        uint64_t compressedSize = 0;
        uint64_t compressionRatio = 0;  // Percentage

        uint32_t chunkCount = 0;
        uint32_t fileCount = 0;

        std::string compressionMethod;  // "Zstandard"
        std::string encryptionMethod;   // "AES-256-GCM"
        std::string hashAlgorithm;      // "BLAKE3" or "SHA-256"

        uint32_t formatVersion = 1;
        std::chrono::system_clock::time_point creationDate;
        std::string createdBy;          // "NotY Repacker v1.0"
    };

    class Manifest {
    public:
        Manifest() = default;
        explicit Manifest(const PackageInfo& info);

        // Package info
        const PackageInfo& getPackageInfo() const { return m_packageInfo; }
        void setPackageInfo(const PackageInfo& info) { m_packageInfo = info; }

        // File entries
        const std::vector<FileEntry>& getFiles() const { return m_files; }
        void addFile(const FileEntry& file);
        void setFiles(const std::vector<FileEntry>& files);
        void clearFiles();

        // Chunks
        const std::vector<ChunkInfo>& getChunks() const { return m_chunks; }
        void addChunk(const ChunkInfo& chunk);
        void setChunks(const std::vector<ChunkInfo>& chunks);

        // Components
        const std::vector<ComponentInfo>& getComponents() const { return m_components; }
        void addComponent(const ComponentInfo& component);
        void setComponents(const std::vector<ComponentInfo>& components);

        // Get file by path
        const FileEntry* getFile(const std::string& path) const;

        // Calculate total sizes
        uint64_t calculateTotalFileSize() const;
        uint64_t calculateTotalCompressedSize() const;

        // Validation
        bool validate() const;
        std::vector<std::string> validateFiles() const;

    private:
        PackageInfo m_packageInfo;
        std::vector<FileEntry> m_files;
        std::vector<ChunkInfo> m_chunks;
        std::vector<ComponentInfo> m_components;

        // Fast lookup
        std::unordered_map<std::string, size_t> m_fileIndex;
    };

} // namespace noty