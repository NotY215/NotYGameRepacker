#pragma once
#include "../common/Common.h"
#include <cstdint>
#include <atomic>
#include <memory>

namespace noty {

    /**
     * @brief Manages system resources for optimal performance
     *
     * Adapts buffer sizes and thread counts based on available RAM.
     * Provides memory-aware configuration for all operations.
     */
    class ResourceManager {
    public:
        enum class MemoryProfile {
            Conservative,   // < 8GB RAM
            Moderate,       // 8-16GB RAM
            High,          // 16-32GB RAM
            Aggressive     // 32+GB RAM
        };

        struct Config {
            size_t compressionBufferSize = 0;
            size_t decompressionBufferSize = 0;
            size_t encryptionBufferSize = 0;
            size_t fileBufferSize = 0;
            size_t chunkBufferSize = 0;
            size_t threadPoolSize = 0;
            MemoryProfile profile = MemoryProfile::Moderate;

            // Adaptive settings
            bool adaptiveBuffers = true;
            bool adaptiveThreads = true;
        };

        static ResourceManager& instance();

        /**
         * @brief Initialize the resource manager
         * @param config Custom configuration (empty = auto-detect)
         */
        void initialize(const Config& config = Config());

        /**
         * @brief Get the current configuration
         */
        const Config& getConfig() const { return m_config; }

        /**
         * @brief Get recommended compression buffer size
         */
        size_t getCompressionBufferSize() const { return m_config.compressionBufferSize; }

        /**
         * @brief Get recommended decompression buffer size
         */
        size_t getDecompressionBufferSize() const { return m_config.decompressionBufferSize; }

        /**
         * @brief Get recommended encryption buffer size
         */
        size_t getEncryptionBufferSize() const { return m_config.encryptionBufferSize; }

        /**
         * @brief Get recommended file buffer size
         */
        size_t getFileBufferSize() const { return m_config.fileBufferSize; }

        /**
         * @brief Get recommended chunk buffer size
         */
        size_t getChunkBufferSize() const { return m_config.chunkBufferSize; }

        /**
         * @brief Get recommended thread pool size
         */
        size_t getThreadPoolSize() const { return m_config.threadPoolSize; }

        /**
         * @brief Get the memory profile
         */
        MemoryProfile getMemoryProfile() const { return m_config.profile; }

        /**
         * @brief Get total system memory in bytes
         */
        uint64_t getTotalSystemMemory() const { return m_totalMemory; }

        /**
         * @brief Get available system memory in bytes
         */
        uint64_t getAvailableMemory() const;

        /**
         * @brief Get CPU core count
         */
        uint32_t getCpuCoreCount() const { return m_cpuCores; }

        /**
         * @brief Check if system is memory-constrained
         */
        bool isMemoryConstrained() const;

        /**
         * @brief Update configuration based on current system state
         */
        void updateConfig();

        /**
         * @brief Get a human-readable description of the current profile
         */
        std::string getProfileDescription() const;

        /**
         * @brief Calculate optimal chunk size based on available memory
         */
        uint64_t calculateOptimalChunkSize(uint64_t requestedSize) const;

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        void detectSystemResources();
        void calculateBufferSizes();
        void calculateThreadPoolSize();

        Config m_config;
        uint64_t m_totalMemory = 0;
        uint32_t m_cpuCores = 0;
        MemoryProfile m_detectedProfile = MemoryProfile::Moderate;
        std::atomic<bool> m_initialized{ false };
    };

} // namespace noty