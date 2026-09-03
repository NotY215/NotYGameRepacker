#pragma once
#include "../common/Common.h"
#include "../package/Manifest.h"
#include <string>
#include <atomic>
#include <functional>
#include <chrono>
#include <vector>

namespace noty {

    /**
     * @brief Represents an installation job configuration and state
     */
    class InstallJob {
    public:
        using ProgressCallback = std::function<void(int percent, const std::string& status)>;

        enum class State {
            Idle,
            Validating,
            Preparing,
            Extracting,
            Verifying,
            Complete,
            Failed,
            Cancelled
        };

        struct Configuration {
            std::string packagePath;           // Path to .noty package or manifest
            std::string installDirectory;      // Where to install the game
            std::string gameName;              // Game name for display
            bool verifyFiles = true;           // Verify file integrity after extraction
            bool createDesktopShortcut = false;
            bool createStartMenuShortcut = false;
            std::vector<std::string> selectedComponents; // Optional components to install

            // Encryption settings (must match package)
            bool enableEncryption = false;
            std::vector<uint8_t> encryptionKey;
            std::vector<uint8_t> encryptionNonce;
        };

        InstallJob();
        explicit InstallJob(const Configuration& config);
        ~InstallJob();

        // Non-copyable
        InstallJob(const InstallJob&) = delete;
        InstallJob& operator=(const InstallJob&) = delete;

        // Movable
        InstallJob(InstallJob&& other) noexcept;
        InstallJob& operator=(InstallJob&& other) noexcept;

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

        uint64_t getInstalledSize() const { return m_installedSize; }
        void setInstalledSize(uint64_t size) { m_installedSize = size; }

        uint64_t getExtractedFileCount() const { return m_extractedFileCount; }
        void setExtractedFileCount(uint64_t count) { m_extractedFileCount = count; }

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

        // Component selection
        bool isComponentSelected(const std::string& componentName) const;
        void selectComponent(const std::string& componentName);
        void deselectComponent(const std::string& componentName);

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
        uint64_t m_installedSize = 0;
        uint64_t m_extractedFileCount = 0;

        ProgressCallback m_progressCallback;
        std::vector<std::string> m_selectedComponents;
    };

} // namespace noty