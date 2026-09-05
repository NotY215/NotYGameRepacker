#pragma once
#include <chrono>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>

namespace noty {

    /**
     * @brief Monitors performance metrics for optimization
     *
     * Tracks CPU usage, memory usage, throughput, and ETA calculations.
     * Provides real-time performance feedback for UI updates.
     */
    class PerformanceMonitor {
    public:
        struct Metrics {
            uint64_t totalBytesProcessed = 0;
            uint64_t totalFilesProcessed = 0;
            uint64_t currentThroughput = 0;     // Bytes per second
            double averageThroughput = 0.0;      // Bytes per second
            uint64_t estimatedTimeRemaining = 0; // Milliseconds
            double cpuUsagePercent = 0.0;
            uint64_t memoryUsageBytes = 0;
            uint64_t peakMemoryUsage = 0;
            uint64_t elapsedMilliseconds = 0;
            uint64_t estimatedTotalTime = 0;
        };

        struct OperationStats {
            std::string operationName;
            uint64_t totalBytes = 0;
            uint64_t totalFiles = 0;
            uint64_t durationMs = 0;
            uint64_t throughput = 0;
        };

        static PerformanceMonitor& instance();

        /**
         * @brief Start monitoring an operation
         * @param operationName Name of the operation
         * @param totalBytes Total bytes to process (for ETA)
         * @param totalFiles Total files to process
         */
        void startOperation(const std::string& operationName,
            uint64_t totalBytes = 0,
            uint64_t totalFiles = 0);

        /**
         * @brief Update progress
         * @param bytesProcessed Bytes processed so far
         * @param filesProcessed Files processed so far
         */
        void updateProgress(uint64_t bytesProcessed, uint64_t filesProcessed = 0);

        /**
         * @brief Update total bytes (for dynamic ETA)
         * @param totalBytes New total bytes
         */
        void updateTotalBytes(uint64_t totalBytes);

        /**
         * @brief Stop the current operation
         */
        void stopOperation();

        /**
         * @brief Get current metrics
         */
        Metrics getMetrics() const;

        /**
         * @brief Get all operation statistics
         */
        std::vector<OperationStats> getAllStats() const;

        /**
         * @brief Get ETA as a formatted string
         */
        std::string getETAString() const;

        /**
         * @brief Get throughput as a formatted string
         */
        std::string getThroughputString() const;

        /**
         * @brief Clear all statistics
         */
        void clearStats();

        /**
         * @brief Update system metrics (CPU, memory)
         */
        void updateSystemMetrics();

        /**
         * @brief Check if ETA is stable (for UI display)
         */
        bool isETAStable() const;

        /**
         * @brief Get the last error message
         */
        std::string getLastError() const { return m_lastError; }

    private:
        PerformanceMonitor() = default;
        ~PerformanceMonitor() = default;
        PerformanceMonitor(const PerformanceMonitor&) = delete;
        PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

        void calculateThroughput();
        void calculateETA();
        void updateSystemMemory();
        void updateCpuUsage();

        // Current operation
        std::string m_operationName;
        std::chrono::steady_clock::time_point m_startTime;
        std::chrono::steady_clock::time_point m_lastUpdateTime;
        uint64_t m_totalBytes = 0;
        uint64_t m_totalFiles = 0;
        uint64_t m_bytesProcessed = 0;
        uint64_t m_filesProcessed = 0;

        // Throughput tracking
        struct ThroughputSample {
            uint64_t bytes;
            uint64_t elapsedMs;
        };
        std::vector<ThroughputSample> m_throughputSamples;
        static constexpr size_t MAX_SAMPLES = 20;

        // Metrics - using std::atomic with direct initialization
        std::atomic<uint64_t> m_currentThroughput{0};
        std::atomic<double> m_averageThroughput{0.0};
        std::atomic<uint64_t> m_estimatedTimeRemaining{0};
        std::atomic<double> m_cpuUsage{0.0};
        std::atomic<uint64_t> m_memoryUsage{0};
        std::atomic<uint64_t> m_peakMemoryUsage{0};
        std::atomic<bool> m_operationActive{false};
        std::atomic<bool> m_etaStable{false};

        // Statistics
        std::vector<OperationStats> m_allStats;
        mutable std::mutex m_statsMutex;

        // Error handling
        std::string m_lastError;

        // System metrics
        uint64_t m_lastMemoryCheck = 0;
        std::chrono::steady_clock::time_point m_lastCpuCheck;
    };

} // namespace noty