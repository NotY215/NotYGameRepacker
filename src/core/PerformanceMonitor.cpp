#include "noty/core/PerformanceMonitor.h"
#include "noty/common/Logger.h"
#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "psapi.lib")

namespace noty {

    PerformanceMonitor& PerformanceMonitor::instance() {
        static PerformanceMonitor instance;
        return instance;
    }

    void PerformanceMonitor::startOperation(const std::string& operationName,
        uint64_t totalBytes,
        uint64_t totalFiles) {
        std::lock_guard<std::mutex> lock(m_statsMutex);

        m_operationName = operationName;
        m_totalBytes = totalBytes;
        m_totalFiles = totalFiles;
        m_bytesProcessed = 0;
        m_filesProcessed = 0;
        m_startTime = std::chrono::steady_clock::now();
        m_lastUpdateTime = m_startTime;
        m_throughputSamples.clear();
        m_operationActive = true;
        m_etaStable = false;

        m_currentThroughput = 0;
        m_averageThroughput = 0.0;
        m_estimatedTimeRemaining = 0;

        Logger::instance().info("Operation started: " + operationName);
    }

    void PerformanceMonitor::updateProgress(uint64_t bytesProcessed, uint64_t filesProcessed) {
        if (!m_operationActive) {
            return;
        }

        m_bytesProcessed = bytesProcessed;
        if (filesProcessed > 0) {
            m_filesProcessed = filesProcessed;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastUpdateTime).count();

        if (elapsed >= 500) {
            calculateThroughput();
            calculateETA();
            m_lastUpdateTime = now;
        }

        static int updateCounter = 0;
        if (++updateCounter % 10 == 0) {
            updateSystemMetrics();
        }
    }

    void PerformanceMonitor::updateTotalBytes(uint64_t totalBytes) {
        m_totalBytes = totalBytes;
        calculateETA();
    }

    void PerformanceMonitor::stopOperation() {
        if (!m_operationActive) {
            return;
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - m_startTime).count();

        std::lock_guard<std::mutex> lock(m_statsMutex);

        OperationStats stats;
        stats.operationName = m_operationName;
        stats.totalBytes = m_bytesProcessed;
        stats.totalFiles = m_filesProcessed;
        stats.durationMs = duration;
        stats.throughput = m_currentThroughput.load();

        m_allStats.push_back(stats);
        m_operationActive = false;

        Logger::instance().info("Operation completed: " + m_operationName +
            " (" + std::to_string(duration) + "ms, " +
            std::to_string(m_currentThroughput / 1024) + " KB/s)");
    }

    PerformanceMonitor::Metrics PerformanceMonitor::getMetrics() const {
        Metrics metrics;
        metrics.totalBytesProcessed = m_bytesProcessed;
        metrics.totalFilesProcessed = m_filesProcessed;
        metrics.currentThroughput = m_currentThroughput.load();
        metrics.averageThroughput = m_averageThroughput.load();
        metrics.estimatedTimeRemaining = m_estimatedTimeRemaining.load();
        metrics.cpuUsagePercent = m_cpuUsage.load();
        metrics.memoryUsageBytes = m_memoryUsage.load();
        metrics.peakMemoryUsage = m_peakMemoryUsage.load();

        auto now = std::chrono::steady_clock::now();
        metrics.elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_startTime).count();

        double avgThroughput = m_averageThroughput.load();
        if (avgThroughput > 0 && m_totalBytes > 0) {
            metrics.estimatedTotalTime = static_cast<uint64_t>(
                m_totalBytes / avgThroughput * 1000);
        }
        else {
            metrics.estimatedTotalTime = 0;
        }

        return metrics;
    }

    std::vector<PerformanceMonitor::OperationStats> PerformanceMonitor::getAllStats() const {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        return m_allStats;
    }

    std::string PerformanceMonitor::getETAString() const {
        uint64_t remaining = m_estimatedTimeRemaining.load();
        if (remaining == 0) {
            return "Calculating...";
        }

        uint64_t seconds = remaining / 1000;
        uint64_t minutes = seconds / 60;
        uint64_t hours = minutes / 60;

        if (hours > 0) {
            return std::to_string(hours) + "h " +
                std::to_string(minutes % 60) + "m " +
                std::to_string(seconds % 60) + "s";
        }
        else if (minutes > 0) {
            return std::to_string(minutes) + "m " +
                std::to_string(seconds % 60) + "s";
        }
        else {
            return std::to_string(seconds) + "s";
        }
    }

    std::string PerformanceMonitor::getThroughputString() const {
        uint64_t throughput = m_currentThroughput.load();

        if (throughput > 1024 * 1024 * 1024) {
            return std::to_string(throughput / (1024 * 1024 * 1024)) + " GB/s";
        }
        else if (throughput > 1024 * 1024) {
            return std::to_string(throughput / (1024 * 1024)) + " MB/s";
        }
        else if (throughput > 1024) {
            return std::to_string(throughput / 1024) + " KB/s";
        }
        else {
            return std::to_string(throughput) + " B/s";
        }
    }

    void PerformanceMonitor::clearStats() {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_allStats.clear();
        m_throughputSamples.clear();
    }

    void PerformanceMonitor::updateSystemMetrics() {
        updateSystemMemory();
        updateCpuUsage();
    }

    bool PerformanceMonitor::isETAStable() const {
        return m_etaStable.load(std::memory_order_acquire);
    }

    void PerformanceMonitor::calculateThroughput() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastUpdateTime).count();

        if (elapsed == 0) {
            return;
        }

        static uint64_t lastBytes = 0;
        uint64_t delta = m_bytesProcessed - lastBytes;
        lastBytes = m_bytesProcessed;

        uint64_t throughput = static_cast<uint64_t>((double)delta / (elapsed / 1000.0));
        m_currentThroughput = throughput;

        m_throughputSamples.push_back({ delta, static_cast<uint64_t>(elapsed) });
        if (m_throughputSamples.size() > MAX_SAMPLES) {
            m_throughputSamples.erase(m_throughputSamples.begin());
        }

        uint64_t totalBytes = 0;
        uint64_t totalTime = 0;
        for (const auto& sample : m_throughputSamples) {
            totalBytes += sample.bytes;
            totalTime += sample.elapsedMs;
        }

        if (totalTime > 0) {
            double avgThroughput = (double)totalBytes / (totalTime / 1000.0);
            m_averageThroughput.store(avgThroughput);
        }
    }

    void PerformanceMonitor::calculateETA() {
        double avgThroughput = m_averageThroughput.load();
        if (avgThroughput <= 0 || m_totalBytes == 0) {
            m_estimatedTimeRemaining = 0;
            m_etaStable = false;
            return;
        }

        uint64_t remaining = m_totalBytes - m_bytesProcessed;
        uint64_t etaMs = static_cast<uint64_t>(remaining / avgThroughput * 1000);
        m_estimatedTimeRemaining = etaMs;

        m_etaStable.store(m_throughputSamples.size() >= 5, std::memory_order_release);
    }

    void PerformanceMonitor::updateSystemMemory() {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
            uint64_t memory = pmc.WorkingSetSize;
            m_memoryUsage.store(memory);

            uint64_t currentPeak = m_peakMemoryUsage.load();
            if (memory > currentPeak) {
                m_peakMemoryUsage.store(memory);
            }
        }
    }

    void PerformanceMonitor::updateCpuUsage() {
        FILETIME idleTime, kernelTime, userTime;
        if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
            static uint64_t lastIdle = 0, lastKernel = 0, lastUser = 0;

            uint64_t idle = (static_cast<uint64_t>(idleTime.dwHighDateTime) << 32) |
                idleTime.dwLowDateTime;
            uint64_t kernel = (static_cast<uint64_t>(kernelTime.dwHighDateTime) << 32) |
                kernelTime.dwLowDateTime;
            uint64_t user = (static_cast<uint64_t>(userTime.dwHighDateTime) << 32) |
                userTime.dwLowDateTime;

            if (lastIdle != 0) {
                uint64_t total = (kernel + user) - (lastKernel + lastUser);
                uint64_t idleDelta = idle - lastIdle;

                if (total > 0) {
                    double usage = 100.0 - (100.0 * idleDelta / total);
                    double clamped = std::max(0.0, std::min(100.0, usage));
                    m_cpuUsage.store(clamped);
                }
            }

            lastIdle = idle;
            lastKernel = kernel;
            lastUser = user;
        }
    }

} // namespace noty