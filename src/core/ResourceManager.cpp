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

        const uint64_t ONE_GIGABYTE = 1024ULL * 1024 * 1024;
        if (m_totalMemory < 8ULL * ONE_GIGABYTE) {
            m_config.profile = MemoryProfile::Conservative;
        }
        else if (m_totalMemory < 16ULL * ONE_GIGABYTE) {
            m_config.profile = MemoryProfile::Moderate;
        }
        else if (m_totalMemory < 32ULL * ONE_GIGABYTE) {
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
        const uint64_t GIGABYTE = 1024ULL * 1024 * 1024;
        const uint64_t MEGABYTE = 1024ULL * 1024;

        if (m_totalMemory < 8ULL * GIGABYTE) {
            m_config.compressionBufferSize = 512 * 1024;
            m_config.decompressionBufferSize = 512 * 1024;
            m_config.encryptionBufferSize = 512 * 1024;
            m_config.fileBufferSize = 512 * 1024;
            m_config.chunkBufferSize = 5 * MEGABYTE;
        }
        else if (m_totalMemory < 16ULL * GIGABYTE) {
            m_config.compressionBufferSize = 1 * MEGABYTE;
            m_config.decompressionBufferSize = 1 * MEGABYTE;
            m_config.encryptionBufferSize = 1 * MEGABYTE;
            m_config.fileBufferSize = 1 * MEGABYTE;
            m_config.chunkBufferSize = 10 * MEGABYTE;
        }
        else if (m_totalMemory < 32ULL * GIGABYTE) {
            m_config.compressionBufferSize = 2 * MEGABYTE;
            m_config.decompressionBufferSize = 2 * MEGABYTE;
            m_config.encryptionBufferSize = 2 * MEGABYTE;
            m_config.fileBufferSize = 2 * MEGABYTE;
            m_config.chunkBufferSize = 25 * MEGABYTE;
        }
        else {
            m_config.compressionBufferSize = 4 * MEGABYTE;
            m_config.decompressionBufferSize = 4 * MEGABYTE;
            m_config.encryptionBufferSize = 4 * MEGABYTE;
            m_config.fileBufferSize = 4 * MEGABYTE;
            m_config.chunkBufferSize = 50 * MEGABYTE;
        }

        const size_t MAX_BUFFER_SIZE = 16 * MEGABYTE;
        m_config.compressionBufferSize = std::min(m_config.compressionBufferSize, MAX_BUFFER_SIZE);
        m_config.decompressionBufferSize = std::min(m_config.decompressionBufferSize, MAX_BUFFER_SIZE);
        m_config.encryptionBufferSize = std::min(m_config.encryptionBufferSize, MAX_BUFFER_SIZE);
        m_config.fileBufferSize = std::min(m_config.fileBufferSize, MAX_BUFFER_SIZE);
        m_config.chunkBufferSize = std::min(m_config.chunkBufferSize, 100 * MEGABYTE);
    }

    void ResourceManager::calculateThreadPoolSize() {
        const uint64_t GIGABYTE = 1024ULL * 1024 * 1024;

        if (m_totalMemory < 8ULL * GIGABYTE) {
            m_config.threadPoolSize = std::max(2u, m_cpuCores / 2);
        }
        else if (m_totalMemory < 16ULL * GIGABYTE) {
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

        const uint64_t MEGABYTE = 1024 * 1024;
        optimal = (optimal / MEGABYTE) * MEGABYTE;

        if (optimal < 100ULL * MEGABYTE) {
            optimal = 100ULL * MEGABYTE;
        }

        return optimal;
    }

} // namespace noty