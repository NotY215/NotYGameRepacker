#include "noty/repacker/RepackJob.h"
#include "noty/common/Logger.h"
#include "noty/common/Constants.h"

namespace noty {

    RepackJob::RepackJob()
        : m_state(State::Idle)
        , m_progress(0)
        , m_cancelled(false)
        , m_originalSize(0)
        , m_compressedSize(0)
    {
        // Generate a default package ID
        m_config.packageId = "NOTY-" + std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
    }

    RepackJob::RepackJob(const Configuration& config)
        : m_config(config)
        , m_state(State::Idle)
        , m_progress(0)
        , m_cancelled(false)
        , m_originalSize(0)
        , m_compressedSize(0)
    {
    }

    RepackJob::~RepackJob() = default;

    RepackJob::RepackJob(RepackJob&& other) noexcept
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
        , m_originalSize(other.m_originalSize)
        , m_compressedSize(other.m_compressedSize)
        , m_outputFiles(std::move(other.m_outputFiles))
        , m_progressCallback(std::move(other.m_progressCallback))
    {
    }

    RepackJob& RepackJob::operator=(RepackJob&& other) noexcept {
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
            m_originalSize = other.m_originalSize;
            m_compressedSize = other.m_compressedSize;
            m_outputFiles = std::move(other.m_outputFiles);
            m_progressCallback = std::move(other.m_progressCallback);
        }
        return *this;
    }

    std::string RepackJob::getStateString() const {
        switch (m_state.load()) {
        case State::Idle: return "Idle";
        case State::Preparing: return "Preparing";
        case State::Scanning: return "Scanning";
        case State::Hashing: return "Hashing";
        case State::Compressing: return "Compressing";
        case State::Encrypting: return "Encrypting";
        case State::WritingManifest: return "Writing Manifest";
        case State::GeneratingSetup: return "Generating Setup";
        case State::Complete: return "Complete";
        case State::Failed: return "Failed";
        case State::Cancelled: return "Cancelled";
        default: return "Unknown";
        }
    }

    uint64_t RepackJob::getElapsedMilliseconds() const {
        auto end = m_endTime;
        if (end == std::chrono::steady_clock::time_point()) {
            end = std::chrono::steady_clock::now();
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            end - m_startTime).count();
    }

    void RepackJob::notifyProgress(int percent, const std::string& status) {
        setProgress(percent);
        setStatus(status);
        if (m_progressCallback) {
            m_progressCallback(percent, status);
        }
    }

    bool RepackJob::validate() const {
        if (m_config.sourceDirectory.empty()) {
            m_validationError = "Source directory is empty";
            return false;
        }

        if (m_config.outputDirectory.empty()) {
            m_validationError = "Output directory is empty";
            return false;
        }

        if (m_config.gameName.empty()) {
            m_validationError = "Game name is empty";
            return false;
        }

        if (m_config.packageId.empty()) {
            m_validationError = "Package ID is empty";
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

        if (m_config.compressionLevel < 1 || m_config.compressionLevel > 22) {
            m_validationError = "Compression level must be between 1 and 22";
            return false;
        }

        if (m_config.maxChunkSize < 1024 * 1024) { // 1MB minimum
            m_validationError = "Maximum chunk size must be at least 1MB";
            return false;
        }

        m_validationError.clear();
        return true;
    }

} // namespace noty