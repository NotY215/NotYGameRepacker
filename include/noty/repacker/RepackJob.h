#pragma once
#include "../common/Common.h"
#include "../package/Manifest.h"
#include <string>
#include <atomic>
#include <functional>
#include <chrono>

namespace noty {

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

            int compressionLevel = 19;
            size_t compressionBufferSize = 1024 * 1024;

            bool enableEncryption = false;  // Default to false

            uint64_t maxChunkSize = 1024 * 1024 * 1024;
            uint32_t maxChunkCount = 0;

            std::string coverImagePath;
            std::vector<ComponentInfo> components;
            std::string hashAlgorithm = "BLAKE3";

            bool includeHiddenFiles = false;
            bool generateSetup = true;
        };

        RepackJob();
        explicit RepackJob(const Configuration& config);
        ~RepackJob();

        RepackJob(const RepackJob&) = delete;
        RepackJob& operator=(const RepackJob&) = delete;

        RepackJob(RepackJob&& other) noexcept;
        RepackJob& operator=(RepackJob&& other) noexcept;

        const Configuration& getConfiguration() const { return m_config; }
        void setConfiguration(const Configuration& config) { m_config = config; }

        State getState() const { return m_state.load(std::memory_order_acquire); }
        void setState(State state) { m_state.store(state, std::memory_order_release); }
        std::string getStateString() const;

        int getProgress() const { return m_progress.load(std::memory_order_acquire); }
        void setProgress(int percent) { m_progress.store(percent, std::memory_order_release); }

        std::string getStatus() const { return m_status; }
        void setStatus(const std::string& status) { m_status = status; }

        std::chrono::steady_clock::time_point getStartTime() const { return m_startTime; }
        void setStartTime(std::chrono::steady_clock::time_point time) { m_startTime = time; }

        std::chrono::steady_clock::time_point getEndTime() const { return m_endTime; }
        void setEndTime(std::chrono::steady_clock::time_point time) { m_endTime = time; }

        uint64_t getElapsedMilliseconds() const;

        const Manifest& getManifest() const { return m_manifest; }
        Manifest& getManifest() { return m_manifest; }

        uint64_t getOriginalSize() const { return m_originalSize; }
        void setOriginalSize(uint64_t size) { m_originalSize = size; }

        uint64_t getCompressedSize() const { return m_compressedSize; }
        void setCompressedSize(uint64_t size) { m_compressedSize = size; }

        const std::vector<std::string>& getOutputFiles() const { return m_outputFiles; }
        void addOutputFile(const std::string& file) { m_outputFiles.push_back(file); }

        std::string getLastError() const { return m_lastError; }
        void setLastError(const std::string& error) { m_lastError = error; }

        void cancel() { m_cancelled.store(true, std::memory_order_release); }
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
        void notifyProgress(int percent, const std::string& status);

        bool validate() const;
        std::string getValidationError() const { return m_validationError; }

    private:
        Configuration m_config;
        std::atomic<State> m_state;
        std::atomic<int> m_progress;
        std::string m_status;
        std::string m_lastError;
        mutable std::string m_validationError;
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