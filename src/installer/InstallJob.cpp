#include "noty/installer/InstallJob.h"
#include "noty/common/Logger.h"
#include <algorithm>

namespace noty {

    InstallJob::InstallJob()
        : m_state(State::Idle)
        , m_progress(0)
        , m_cancelled(false)
        , m_installedSize(0)
        , m_extractedFileCount(0)
    {
    }

    InstallJob::InstallJob(const Configuration& config)
        : m_config(config)
        , m_state(State::Idle)
        , m_progress(0)
        , m_cancelled(false)
        , m_installedSize(0)
        , m_extractedFileCount(0)
    {
        // Initialize selected components from config
        m_selectedComponents = config.selectedComponents;
    }

    InstallJob::~InstallJob() = default;

    InstallJob::InstallJob(InstallJob&& other) noexcept
        : m_config(std::move(other.m_config))
        , m_state(other.m_state.load())
        , m_progress(other.m_progress.load())
        , m_status(std::move(other.m_status))
        , m_lastError(std::move(other.m_lastError))
        , m_validationError(std::move(other.m_validationError))
        , m_cancelled(other.m_cancelled.load())
        , m_startTime(other.m_startTime)
        , m_endTime(other.m_endTime)
        , m_manifest(std::move(other.m_manifest))
        , m_installedSize(other.m_installedSize)
        , m_extractedFileCount(other.m_extractedFileCount)
        , m_progressCallback(std::move(other.m_progressCallback))
        , m_selectedComponents(std::move(other.m_selectedComponents))
    {
    }

    InstallJob& InstallJob::operator=(InstallJob&& other) noexcept {
        if (this != &other) {
            m_config = std::move(other.m_config);
            m_state.store(other.m_state.load());
            m_progress.store(other.m_progress.load());
            m_status = std::move(other.m_status);
            m_lastError = std::move(other.m_lastError);
            m_validationError = std::move(other.m_validationError);
            m_cancelled.store(other.m_cancelled.load());
            m_startTime = other.m_startTime;
            m_endTime = other.m_endTime;
            m_manifest = std::move(other.m_manifest);
            m_installedSize = other.m_installedSize;
            m_extractedFileCount = other.m_extractedFileCount;
            m_progressCallback = std::move(other.m_progressCallback);
            m_selectedComponents = std::move(other.m_selectedComponents);
        }
        return *this;
    }

    std::string InstallJob::getStateString() const {
        switch (m_state.load()) {
        case State::Idle: return "Idle";
        case State::Validating: return "Validating";
        case State::Preparing: return "Preparing";
        case State::Extracting: return "Extracting";
        case State::Verifying: return "Verifying";
        case State::Complete: return "Complete";
        case State::Failed: return "Failed";
        case State::Cancelled: return "Cancelled";
        default: return "Unknown";
        }
    }

    uint64_t InstallJob::getElapsedMilliseconds() const {
        auto end = m_endTime;
        if (end == std::chrono::steady_clock::time_point()) {
            end = std::chrono::steady_clock::now();
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            end - m_startTime).count();
    }

    void InstallJob::notifyProgress(int percent, const std::string& status) {
        setProgress(percent);
        setStatus(status);
        if (m_progressCallback) {
            m_progressCallback(percent, status);
        }
    }

    bool InstallJob::validate() const {
        if (m_config.packagePath.empty()) {
            m_validationError = "Package path is empty";
            return false;
        }

        if (m_config.installDirectory.empty()) {
            m_validationError = "Install directory is empty";
            return false;
        }

        if (m_config.gameName.empty()) {
            m_validationError = "Game name is empty";
            return false;
        }

        if (m_config.enableEncryption) {
            if (m_config.encryptionKey.size() != 32) {
                m_validationError = "Encryption key must be 32 bytes for AES-256";
                return false;
            }
            if (m_config.encryptionNonce.size() != 12) {
                m_validationError = "Encryption nonce must be 12 bytes for AES-GCM";
                return false;
            }
        }

        m_validationError.clear();
        return true;
    }

    bool InstallJob::isComponentSelected(const std::string& componentName) const {
        return std::find(m_selectedComponents.begin(), m_selectedComponents.end(),
            componentName) != m_selectedComponents.end();
    }

    void InstallJob::selectComponent(const std::string& componentName) {
        if (!isComponentSelected(componentName)) {
            m_selectedComponents.push_back(componentName);
        }
    }

    void InstallJob::deselectComponent(const std::string& componentName) {
        auto it = std::find(m_selectedComponents.begin(), m_selectedComponents.end(),
            componentName);
        if (it != m_selectedComponents.end()) {
            m_selectedComponents.erase(it);
        }
    }

} // namespace noty