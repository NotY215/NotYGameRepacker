================================================================================
FILE: include / noty / installer / InstallEngine.h
================================================================================
#pragma once
#include "InstallJob.h"
#include "../common/Common.h"
#include <memory>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace noty {

    class ExtractionEngine;

    /**
     * @brief Manages installation operations with multi-threaded support
     */
    class InstallEngine {
    public:
        InstallEngine();
        ~InstallEngine();

        // Non-copyable
        InstallEngine(const InstallEngine&) = delete;
        InstallEngine& operator=(const InstallEngine&) = delete;

        /**
         * @brief Start an installation job
         * @param job The job configuration
         * @return true if job was started successfully
         */
        bool startJob(InstallJob job);

        /**
         * @brief Cancel the currently running job
         */
        void cancelCurrentJob();

        /**
         * @brief Cancel all queued jobs
         */
        void cancelAllJobs();

        /**
         * @brief Get the current job
         */
        InstallJob* getCurrentJob() { return m_currentJob.get(); }
        const InstallJob* getCurrentJob() const { return m_currentJob.get(); }

        /**
         * @brief Check if engine is busy
         */
        bool isBusy() const;

        /**
         * @brief Get the number of queued jobs
         */
        size_t getQueueSize() const;

        /**
         * @brief Shutdown the engine
         */
        void shutdown();

        /**
         * @brief Wait for all jobs to complete
         * @param timeoutMs Timeout in milliseconds (0 = wait forever)
         * @return true if all jobs completed, false if timeout
         */
        bool waitForCompletion(uint32_t timeoutMs = 0);

        /**
         * @brief Set thread pool size (default = CPU cores)
         */
        void setThreadPoolSize(size_t size) { m_threadPoolSize = size; }

        /**
         * @brief Get thread pool size
         */
        size_t getThreadPoolSize() const { return m_threadPoolSize; }

        /**
         * @brief Get last error message
         */
        std::string getLastError() const { return m_lastError; }

        /**
         * @brief Set extraction buffer size
         */
        void setExtractionBufferSize(size_t size) { m_extractionBufferSize = size; }

    private:
        void workerThread();
        bool processJob(InstallJob& job);
        void cleanup();

        std::unique_ptr<ExtractionEngine> m_extractionEngine;

        std::queue<InstallJob> m_jobQueue;
        std::mutex m_queueMutex;
        std::condition_variable m_queueCondition;

        std::unique_ptr<InstallJob> m_currentJob;
        std::mutex m_currentJobMutex;

        std::vector<std::thread> m_workerThreads;
        std::atomic<bool> m_running;
        std::atomic<bool> m_shutdownRequested;

        size_t m_threadPoolSize;
        size_t m_extractionBufferSize;

        std::string m_lastError;

        std::mutex m_completionMutex;
        std::condition_variable m_completionCondition;
        std::atomic<size_t> m_activeJobs;
        std::atomic<bool> m_processingComplete;
    };

} // namespace noty