================================================================================
FILE: include / noty / repacker / RepackJob.h
================================================================================
#pragma once
#include "../common/Common.h"
#include "../package/Manifest.h"
#include <string>
#include <atomic>
#include <functional>
#include <chrono>

namespace noty {

    /**
     * @brief Represents a repacking job configuration and state
     */
    class RepackJob {
    public:
        using ProgressCallback = std::function<void(int percent, const std::string& status)>;

        enum class State {
            Idle,
            Preparing,
            Scanning,
            Hashing,
            Compressing,
            Encrypting,
            WritingManifest,
            GeneratingSetup,
            Complete,
            Failed,
            Cancelled
        };

        struct Configuration {
            std::string sourceDirectory;
            std::string outputDirectory;
            std::string gameName;
            std::string gameVersion;
            std::string packageId;
            std::string repackerName;
            std::string setupName;

            // Compression settings
            int compressionLevel = 19;
            size_t compressionBufferSize = 1024 * 1024; // 1MB

            // Encryption settings
            bool enableEncryption = true;
            std::vector<uint8_t> encryptionKey;
            std::vector<uint8_t> encryptionNonce;

            // Chunk settings
            uint64_t maxChunkSize = 1024 * 1024 * 1024; // 1GB per chunk
            uint32_t maxChunkCount = 0; // 0 = unlimited

            // Cover image
            std::string coverImagePath;

            // Optional components
            std::vector<ComponentInfo> components;

            // Hash algorithm
            std::string hashAlgorithm = "BLAKE3";

            // Additional settings
            bool includeHiddenFiles = false;
            bool generateSetup = true;
        };

        RepackJob();
        explicit RepackJob(const Configuration& config);
        ~RepackJob();

        // Non-copyable
        RepackJob(const RepackJob&) = delete;
        RepackJob& operator=(const RepackJob&) = delete;

        // Movable
        RepackJob(RepackJob&& other) noexcept;
        RepackJob& operator=(RepackJob&& other) noexcept;

        // Configuration
        const Configuration& getConfiguration() const { return m_config; }
        void setConfiguration(const Configuration& config) { m_config = config; }

        // State management
        State getState() const { return m_state.load(std::memory_order_acquire); }
        void setState(State state) { m_state.store(state, std::memory_order_release); }
        std::string getStateString() const;

        // Progress
        int getProgress() const { return m_progress.load(std::memory_order_acquire); }
        void setProgress(int percent) { m_progress.store(percent, std::memory_order_release); }

        // Status message
        std::string getStatus() const { return m_status; }
        void setStatus(const std::string& status) { m_status = status; }

        // Timing
        std::chrono::steady_clock::time_point getStartTime() const { return m_startTime; }
        void setStartTime(std::chrono::steady_clock::time_point time) { m_startTime = time; }

        std::chrono::steady_clock::time_point getEndTime() const { return m_endTime; }
        void setEndTime(std::chrono::steady_clock::time_point time) { m_endTime = time; }

        uint64_t getElapsedMilliseconds() const;

        // Results
        const Manifest& getManifest() const { return m_manifest; }
        Manifest& getManifest() { return m_manifest; }

        uint64_t getOriginalSize() const { return m_originalSize; }
        void setOriginalSize(uint64_t size) { m_originalSize = size; }

        uint64_t getCompressedSize() const { return m_compressedSize; }
        void setCompressedSize(uint64_t size) { m_compressedSize = size; }

        const std::vector<std::string>& getOutputFiles() const { return m_outputFiles; }
        void addOutputFile(const std::string& file) { m_outputFiles.push_back(file); }

        // Error handling
        std::string getLastError() const { return m_lastError; }
        void setLastError(const std::string& error) { m_lastError = error; }

        // Cancellation
        void cancel() { m_cancelled.store(true, std::memory_order_release); }
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        // Callbacks
        void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
        void notifyProgress(int percent, const std::string& status);

        // Validation
        bool validate() const;
        std::string getValidationError() const { return m_validationError; }

    private:
        Configuration m_config;
        std::atomic<State> m_state;
        std::atomic<int> m_progress;
        std::string m_status;
        std::string m_lastError;
        std::string m_validationError;
        std::atomic<bool> m_cancelled;
        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_endTime;

        Manifest m_manifest;
        uint64_t m_originalSize = 0;
        uint64_t m_compressedSize = 0;
        std::vector<std::string> m_outputFiles;

        ProgressCallback m_progressCallback;
    };

} // namespace noty