#include "noty/core/ResourceManager.h"
#include "noty/common/Logger.h"
#include <windows.h>
#include <algorithm>
#include <thread>

namespace noty {

    ResourceManager& ResourceManager::instance() {
        static ResourceManager instance;
        return instance;
    }

    void ResourceManager::initialize(const Config& config) {
        if (m_initialized.exchange(true)) {
            return;
        }

        detectSystemResources();

        // Copy provided config
        m_config = config;

        // Auto-detect if not explicitly set
        if (m_config.adaptiveBuffers) {
            calculateBufferSizes();
        }
        else {
            // Use provided sizes or fallback to defaults
            if (m_config.compressionBufferSize == 0) m_config.compressionBufferSize = 1024 * 1024;
            if (m_config.decompressionBufferSize == 0) m_config.decompressionBufferSize = 1024 * 1024;
            if (m_config.encryptionBufferSize == 0) m_config.encryptionBufferSize = 1024 * 1024;
            if (m_config.fileBufferSize == 0) m_config.fileBufferSize = 1024 * 1024;
            if (m_config.chunkBufferSize == 0) m_config.chunkBufferSize = 10 * 1024 * 1024;
        }

        if (m_config.adaptiveThreads) {
            calculateThreadPoolSize();
        }
        else if (m_config.threadPoolSize == 0) {
            m_config.threadPoolSize = std::thread::hardware_concurrency();
            if (m_config.threadPoolSize == 0) m_config.threadPoolSize = 4;
        }

        // Determine profile based on detected memory
        if (m_totalMemory < 8ULL * 1024 * 1024 * 1024) {
            m_config.profile = MemoryProfile::Conservative;
        }
        else if (m_totalMemory < 16ULL * 1024 * 1024 * 1024) {
            m_config.profile = MemoryProfile::Moderate;
        }
        else if (m_totalMemory < 32ULL * 1024 * 1024 * 1024) {
            m_config.profile = MemoryProfile::High;
        }
        else {
            m_config.profile = MemoryProfile::Aggressive;
        }

        Logger::instance().info("ResourceManager initialized with profile: " +
            getProfileDescription() +
            " (Total RAM: " + std::to_string(m_totalMemory / (1024 * 1024 * 1024)) + " GB)");
        Logger::instance().info("Thread pool size: " + std::to_string(m_config.threadPoolSize));
        Logger::instance().info("Compression buffer: " +
            std::to_string(m_config.compressionBufferSize / 1024) + " KB");
    }

    void ResourceManager::detectSystemResources() {
        // Get system memory
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            m_totalMemory = memStatus.ullTotalPhys;
        }
        else {
            m_totalMemory = 8ULL * 1024 * 1024 * 1024; // Fallback to 8GB
            Logger::instance().warning("Failed to detect system memory, using default: 8GB");
        }

        // Get CPU cores
        m_cpuCores = std::thread::hardware_concurrency();
        if (m_cpuCores == 0) {
            m_cpuCores = 4;
            Logger::instance().warning("Failed to detect CPU cores, using default: 4");
        }
    }

    void ResourceManager::calculateBufferSizes() {
        const uint64_t GB = 1024ULL * 1024 * 1024;
        const uint64_t MB = 1024ULL * 1024;

        if (m_totalMemory < 8 * GB) {
            // Conservative: < 8GB
            m_config.compressionBufferSize = 512 * 1024;      // 512KB
            m_config.decompressionBufferSize = 512 * 1024;    // 512KB
            m_config.encryptionBufferSize = 512 * 1024;       // 512KB
            m_config.fileBufferSize = 512 * 1024;             // 512KB
            m_config.chunkBufferSize = 5 * MB;                // 5MB
        }
        else if (m_totalMemory < 16 * GB) {
            // Moderate: 8-16GB
            m_config.compressionBufferSize = 1 * MB;          // 1MB
            m_config.decompressionBufferSize = 1 * MB;        // 1MB
            m_config.encryptionBufferSize = 1 * MB;           // 1MB
            m_config.fileBufferSize = 1 * MB;                 // 1MB
            m_config.chunkBufferSize = 10 * MB;               // 10MB
        }
        else if (m_totalMemory < 32 * GB) {
            // High: 16-32GB
            m_config.compressionBufferSize = 2 * MB;          // 2MB
            m_config.decompressionBufferSize = 2 * MB;        // 2MB
            m_config.encryptionBufferSize = 2 * MB;           // 2MB
            m_config.fileBufferSize = 2 * MB;                 // 2MB
            m_config.chunkBufferSize = 25 * MB;               // 25MB
        }
        else {
            // Aggressive: 32+GB
            m_config.compressionBufferSize = 4 * MB;          // 4MB
            m_config.decompressionBufferSize = 4 * MB;        // 4MB
            m_config.encryptionBufferSize = 4 * MB;           // 4MB
            m_config.fileBufferSize = 4 * MB;                 // 4MB
            m_config.chunkBufferSize = 50 * MB;               // 50MB
        }

        // Cap buffer sizes to prevent excessive memory usage
        const size_t MAX_BUFFER = 16 * MB;
        m_config.compressionBufferSize = std::min(m_config.compressionBufferSize, MAX_BUFFER);
        m_config.decompressionBufferSize = std::min(m_config.decompressionBufferSize, MAX_BUFFER);
        m_config.encryptionBufferSize = std::min(m_config.encryptionBufferSize, MAX_BUFFER);
        m_config.fileBufferSize = std::min(m_config.fileBufferSize, MAX_BUFFER);
        m_config.chunkBufferSize = std::min(m_config.chunkBufferSize, 100 * MB);
    }

    void ResourceManager::calculateThreadPoolSize() {
        if (m_totalMemory < 8 * 1024 * 1024 * 1024) {
            // Conservative: Use fewer threads to save memory
            m_config.threadPoolSize = std::max(2u, m_cpuCores / 2);
        }
        else if (m_totalMemory < 16 * 1024 * 1024 * 1024) {
            // Moderate: Use most cores
            m_config.threadPoolSize = std::max(2u, m_cpuCores - 1);
        }
        else {
            // High/Aggressive: Use all cores
            m_config.threadPoolSize = m_cpuCores;
        }

        // Ensure at least 2 threads for I/O overlap
        if (m_config.threadPoolSize < 2) {
            m_config.threadPoolSize = 2;
        }
    }

    uint64_t ResourceManager::getAvailableMemory() const {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            return memStatus.ullAvailPhys;
        }
        return m_totalMemory / 2; // Fallback
    }

    bool ResourceManager::isMemoryConstrained() const {
        uint64_t available = getAvailableMemory();
        uint64_t total = m_totalMemory;
        return (available < total / 4); // Less than 25% available
    }

    void ResourceManager::updateConfig() {
        // Re-detect and recalculate
        detectSystemResources();
        calculateBufferSizes();
        calculateThreadPoolSize();

        Logger::instance().debug("ResourceManager config updated");
    }

    std::string ResourceManager::getProfileDescription() const {
        switch (m_config.profile) {
        case MemoryProfile::Conservative:
            return "Conservative (< 8GB RAM)";
        case MemoryProfile::Moderate:
            return "Moderate (8-16GB RAM)";
        case MemoryProfile::High:
            return "High (16-32GB RAM)";
        case MemoryProfile::Aggressive:
            return "Aggressive (32+GB RAM)";
        default:
            return "Unknown";
        }
    }

    uint64_t ResourceManager::calculateOptimalChunkSize(uint64_t requestedSize) const {
        // Ensure chunk size doesn't exceed available memory / 4
        uint64_t maxSafeChunk = getAvailableMemory() / 4;
        if (maxSafeChunk < 100 * 1024 * 1024) {
            maxSafeChunk = 100 * 1024 * 1024; // Minimum 100MB
        }

        uint64_t optimal = std::min(requestedSize, maxSafeChunk);

        // Round to MB
        const uint64_t MB = 1024 * 1024;
        optimal = (optimal / MB) * MB;

        // Ensure at least 100MB
        if (optimal < 100 * MB) {
            optimal = 100 * MB;
        }

        return optimal;
    }

} // namespace noty