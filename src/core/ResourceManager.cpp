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

        m_config = config;

        if (m_config.adaptiveBuffers) {
            calculateBufferSizes();
        }
        else {
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

        const unsigned long long ONE_GB = 1024ULL * 1024 * 1024;
        if (m_totalMemory < 8ULL * ONE_GB) {
            m_config.profile = MemoryProfile::Conservative;
        }
        else if (m_totalMemory < 16ULL * ONE_GB) {
            m_config.profile = MemoryProfile::Moderate;
        }
        else if (m_totalMemory < 32ULL * ONE_GB) {
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
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        if (GlobalMemoryStatusEx(&memStatus)) {
            m_totalMemory = memStatus.ullTotalPhys;
        }
        else {
            m_totalMemory = 8ULL * 1024 * 1024 * 1024;
            Logger::instance().warning("Failed to detect system memory, using default: 8GB");
        }

        m_cpuCores = std::thread::hardware_concurrency();
        if (m_cpuCores == 0) {
            m_cpuCores = 4;
            Logger::instance().warning("Failed to detect CPU cores, using default: 4");
        }
    }

    void ResourceManager::calculateBufferSizes() {
        const unsigned long long GB = 1024ULL * 1024 * 1024;
        const unsigned long long MB = 1024ULL * 1024;

        if (m_totalMemory < 8ULL * GB) {
            m_config.compressionBufferSize = 512 * 1024;
            m_config.decompressionBufferSize = 512 * 1024;
            m_config.encryptionBufferSize = 512 * 1024;
            m_config.fileBufferSize = 512 * 1024;
            m_config.chunkBufferSize = 5ULL * MB;
        }
        else if (m_totalMemory < 16ULL * GB) {
            m_config.compressionBufferSize = 1ULL * MB;
            m_config.decompressionBufferSize = 1ULL * MB;
            m_config.encryptionBufferSize = 1ULL * MB;
            m_config.fileBufferSize = 1ULL * MB;
            m_config.chunkBufferSize = 10ULL * MB;
        }
        else if (m_totalMemory < 32ULL * GB) {
            m_config.compressionBufferSize = 2ULL * MB;
            m_config.decompressionBufferSize = 2ULL * MB;
            m_config.encryptionBufferSize = 2ULL * MB;
            m_config.fileBufferSize = 2ULL * MB;
            m_config.chunkBufferSize = 25ULL * MB;
        }
        else {
            m_config.compressionBufferSize = 4ULL * MB;
            m_config.decompressionBufferSize = 4ULL * MB;
            m_config.encryptionBufferSize = 4ULL * MB;
            m_config.fileBufferSize = 4ULL * MB;
            m_config.chunkBufferSize = 50ULL * MB;
        }

        const size_t MAX_BUFFER_SIZE = 16ULL * MB;
        m_config.compressionBufferSize = std::min(m_config.compressionBufferSize, MAX_BUFFER_SIZE);
        m_config.decompressionBufferSize = std::min(m_config.decompressionBufferSize, MAX_BUFFER_SIZE);
        m_config.encryptionBufferSize = std::min(m_config.encryptionBufferSize, MAX_BUFFER_SIZE);
        m_config.fileBufferSize = std::min(m_config.fileBufferSize, MAX_BUFFER_SIZE);
        m_config.chunkBufferSize = std::min(m_config.chunkBufferSize, 100ULL * MB);
    }

    void ResourceManager::calculateThreadPoolSize() {
        const unsigned long long GB = 1024ULL * 1024 * 1024;

        if (m_totalMemory < 8ULL * GB) {
            m_config.threadPoolSize = std::max(2u, m_cpuCores / 2);
        }
        else if (m_totalMemory < 16ULL * GB) {
            m_config.threadPoolSize = std::max(2u, m_cpuCores - 1);
        }
        else {
            m_config.threadPoolSize = m_cpuCores;
        }

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
        return m_totalMemory / 2;
    }

    bool ResourceManager::isMemoryConstrained() const {
        uint64_t available = getAvailableMemory();
        uint64_t total = m_totalMemory;
        return (available < total / 4);
    }

    void ResourceManager::updateConfig() {
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
        uint64_t maxSafeChunk = getAvailableMemory() / 4;
        if (maxSafeChunk < 100ULL * 1024 * 1024) {
            maxSafeChunk = 100ULL * 1024 * 1024;
        }

        uint64_t optimal = std::min(requestedSize, maxSafeChunk);

        const uint64_t MB = 1024 * 1024;
        optimal = (optimal / MB) * MB;

        if (optimal < 100ULL * MB) {
            optimal = 100ULL * MB;
        }

        return optimal;
    }

} // namespace noty