#pragma once
#include "../common/Common.h"
#include "../package/Manifest.h"
#include <string>
#include <atomic>
#include <functional>
#include <chrono>
#include <vector>

namespace noty {

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
            std::string packagePath;
            std::string installDirectory;
            std::string gameName;
            bool verifyFiles = true;
            bool createDesktopShortcut = false;
            bool createStartMenuShortcut = false;
            std::vector<std::string> selectedComponents;
        };

        InstallJob();
        explicit InstallJob(const Configuration& config);
        ~InstallJob();

        InstallJob(const InstallJob&) = delete;
        InstallJob& operator=(const InstallJob&) = delete;

        InstallJob(InstallJob&& other) noexcept;
        InstallJob& operator=(InstallJob&& other) noexcept;

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

        uint64_t getInstalledSize() const { return m_installedSize; }
        void setInstalledSize(uint64_t size) { m_installedSize = size; }

        uint64_t getExtractedFileCount() const { return m_extractedFileCount; }
        void setExtractedFileCount(uint64_t count) { m_extractedFileCount = count; }

        std::string getLastError() const { return m_lastError; }
        void setLastError(const std::string& error) { m_lastError = error; }

        void cancel() { m_cancelled.store(true, std::memory_order_release); }
        bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

        void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
        void notifyProgress(int percent, const std::string& status);

        bool validate() const;
        std::string getValidationError() const { return m_validationError; }

        bool isComponentSelected(const std::string& componentName) const;
        void selectComponent(const std::string& componentName);
        void deselectComponent(const std::string& componentName);

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
        uint64_t m_installedSize = 0;
        uint64_t m_extractedFileCount = 0;

        ProgressCallback m_progressCallback;
        std::vector<std::string> m_selectedComponents;
    };

} // namespace noty