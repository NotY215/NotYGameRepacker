================================================================================
FILE: include / noty / repacker / PackageBuilder.h
================================================================================
#pragma once
#include "../common/Common.h"
#include "../package/Manifest.h"
#include "../hashing/Hasher.h"
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

namespace noty {

    class ZstdCompressor;
    class Encryptor;
    class KeyManager;

    /**
     * @brief Builds .noty package files from source directories
     */
    class PackageBuilder {
    public:
        using ProgressCallback = std::function<void(int percent, const std::string& status)>;
        using FileProgressCallback = std::function<void(const std::string& filename, int percent)>;

        PackageBuilder();
        ~PackageBuilder();

        // Non-copyable
        PackageBuilder(const PackageBuilder&) = delete;
        PackageBuilder& operator=(const PackageBuilder&) = delete;

        // Movable
        PackageBuilder(PackageBuilder&& other) noexcept;
        PackageBuilder& operator=(PackageBuilder&& other) noexcept;

        /**
         * @brief Build a package from a source directory
         * @param sourceDirectory Source directory containing game files
         * @param outputDirectory Directory to write package files
         * @param config Package configuration
         * @param manifest Output manifest (will be populated)
         * @param progress Progress callback
         * @param fileProgress File-level progress callback
         * @return true on success
         */
        bool buildPackage(const std::string& sourceDirectory,
            const std::string& outputDirectory,
            const RepackJob::Configuration& config,
            Manifest& manifest,
            ProgressCallback progress = nullptr,
            FileProgressCallback fileProgress = nullptr);

        /**
         * @brief Cancel the current build operation
         */
        void cancel();

        /**
         * @brief Check if operation has been cancelled
         */
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        /**
         * @brief Get the total size of processed files
         */
        uint64_t getTotalProcessedSize() const { return m_totalProcessedSize; }

        /**
         * @brief Get the number of processed files
         */
        uint64_t getProcessedFileCount() const { return m_processedFileCount; }

        /**
         * @brief Get last error message
         */
        std::string getLastError() const { return m_lastError; }

        /**
         * @brief Get generated setup.exe path
         */
        std::string getSetupPath() const { return m_setupPath; }

        /**
         * @brief Get package chunk paths
         */
        const std::vector<std::string>& getChunkPaths() const { return m_chunkPaths; }

    private:
        struct BuildContext {
            std::string sourceDirectory;
            std::string outputDirectory;
            RepackJob::Configuration config;
            Manifest* manifest;
            ProgressCallback progress;
            FileProgressCallback fileProgress;

            uint64_t totalSize = 0;
            uint64_t processedSize = 0;
            uint64_t fileCount = 0;
            uint64_t processedFileCount = 0;

            std::vector<FileInfo> files;
            std::vector<FileEntry> fileEntries;
            std::vector<ChunkInfo> chunks;

            uint32_t currentChunkId = 0;
            uint64_t currentChunkSize = 0;
            std::vector<uint8_t> currentChunkData;

            bool cancelled = false;
        };

        bool scanSourceDirectory(BuildContext& ctx);
        bool calculateHashes(BuildContext& ctx);
        bool buildChunks(BuildContext& ctx);
        bool writeChunks(BuildContext& ctx);
        bool writeManifest(BuildContext& ctx);
        bool generateSetup(BuildContext& ctx);
        bool writeCoverImage(BuildContext& ctx);

        bool compressAndEncryptFile(const FileInfo& fileInfo,
            BuildContext& ctx,
            FileEntry& entry,
            std::vector<uint8_t>& chunkData);

        bool finalizeChunk(BuildContext& ctx);

        void updateProgress(BuildContext& ctx, int percent, const std::string& status);
        void updateFileProgress(BuildContext& ctx, const std::string& filename, int percent);

        std::unique_ptr<ZstdCompressor> m_compressor;
        std::unique_ptr<Encryptor> m_encryptor;
        std::unique_ptr<KeyManager> m_keyManager;
        std::unique_ptr<Hasher> m_hasher;

        std::atomic<bool> m_cancelled;
        std::string m_lastError;

        uint64_t m_totalProcessedSize;
        uint64_t m_processedFileCount;
        std::string m_setupPath;
        std::vector<std::string> m_chunkPaths;

        // Buffer for file operations
        std::vector<uint8_t> m_fileBuffer;
        std::vector<uint8_t> m_compressedBuffer;
        std::vector<uint8_t> m_encryptedBuffer;
    };

} // namespace noty