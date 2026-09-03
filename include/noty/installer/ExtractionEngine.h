#pragma once
#include "../common/Common.h"
#include "../package/Manifest.h"
#include "../installer/InstallJob.h"
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>

namespace noty {

    class ZstdDecompressor;
    class Decryptor;
    class Hasher;

    /**
     * @brief Extracts .noty package files to a target directory
     */
    class ExtractionEngine {
    public:
        using ProgressCallback = std::function<void(int percent, const std::string& status)>;
        using FileProgressCallback = std::function<void(const std::string& filename, int percent)>;

        ExtractionEngine();
        ~ExtractionEngine();

        // Non-copyable
        ExtractionEngine(const ExtractionEngine&) = delete;
        ExtractionEngine& operator=(const ExtractionEngine&) = delete;

        // Movable
        ExtractionEngine(ExtractionEngine&& other) noexcept;
        ExtractionEngine& operator=(ExtractionEngine&& other) noexcept;

        /**
         * @brief Extract a package to the installation directory
         * @param packagePath Path to the package manifest or chunk directory
         * @param installDirectory Target installation directory
         * @param manifest Output manifest (will be populated)
         * @param config Installation configuration
         * @param progress Progress callback
         * @param fileProgress File-level progress callback
         * @return true on success
         */
        bool extractPackage(const std::string& packagePath,
            const std::string& installDirectory,
            Manifest& manifest,
            const InstallJob::Configuration& config,
            ProgressCallback progress = nullptr,
            FileProgressCallback fileProgress = nullptr);

        /**
         * @brief Cancel the current extraction operation
         */
        void cancel();

        /**
         * @brief Check if operation has been cancelled
         */
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        /**
         * @brief Get the total size of extracted files
         */
        uint64_t getExtractedSize() const { return m_extractedSize; }

        /**
         * @brief Get the number of extracted files
         */
        uint64_t getExtractedFileCount() const { return m_extractedFileCount; }

        /**
         * @brief Get last error message
         */
        std::string getLastError() const { return m_lastError; }

        /**
         * @brief Verify extracted files against the manifest
         * @param manifest The manifest to verify against
         * @param installDirectory Directory containing extracted files
         * @return true if all files pass verification
         */
        bool verifyExtraction(const Manifest& manifest, const std::string& installDirectory);

        /**
         * @brief Get the size of a package without extracting
         * @param packagePath Path to the package
         * @return Total size of all files in the package
         */
        uint64_t getPackageSize(const std::string& packagePath);

    private:
        struct ExtractionContext {
            std::string packagePath;
            std::string installDirectory;
            InstallJob::Configuration config;
            Manifest* manifest;
            ProgressCallback progress;
            FileProgressCallback fileProgress;

            uint64_t totalSize = 0;
            uint64_t extractedSize = 0;
            uint64_t fileCount = 0;
            uint64_t extractedFileCount = 0;

            bool cancelled = false;
        };

        bool readManifest(ExtractionContext& ctx);
        bool prepareInstallDirectory(ExtractionContext& ctx);
        bool extractChunks(ExtractionContext& ctx);
        bool extractFileFromChunk(ExtractionContext& ctx,
            const FileEntry& entry,
            const std::vector<uint8_t>& chunkData);
        bool verifyExtractedFiles(ExtractionContext& ctx);

        bool decompressAndDecryptFile(const std::vector<uint8_t>& encryptedData,
            const FileEntry& entry,
            std::vector<uint8_t>& outputData,
            ExtractionContext& ctx);

        void updateProgress(ExtractionContext& ctx, int percent, const std::string& status);
        void updateFileProgress(ExtractionContext& ctx, const std::string& filename, int percent);

        std::unique_ptr<ZstdDecompressor> m_decompressor;
        std::unique_ptr<Decryptor> m_decryptor;
        std::unique_ptr<Hasher> m_hasher;

        std::atomic<bool> m_cancelled;
        std::string m_lastError;

        uint64_t m_extractedSize;
        uint64_t m_extractedFileCount;

        // Buffers for extraction
        std::vector<uint8_t> m_fileBuffer;
        std::vector<uint8_t> m_decompressedBuffer;
        std::vector<uint8_t> m_decryptedBuffer;
    };

} // namespace noty