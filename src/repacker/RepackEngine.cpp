#include "noty/repacker/RepackEngine.h"
#include "noty/repacker/PackageBuilder.h"
#include "noty/common/Logger.h"
#include <thread>
#include <chrono>

namespace noty {

    RepackEngine::RepackEngine()
        : m_running(true)
        , m_shutdownRequested(false)
        , m_threadPoolSize(std::thread::hardware_concurrency())
        , m_compressionBufferSize(1024 * 1024)
        , m_encryptionBufferSize(1024 * 1024)
        , m_activeJobs(0)
        , m_processingComplete(true)
    {
        if (m_threadPoolSize == 0) {
            m_threadPoolSize = 4;
        }

        m_packageBuilder = std::make_unique<PackageBuilder>();

        for (size_t i = 0; i < m_threadPoolSize; ++i) {
            m_workerThreads.emplace_back(&RepackEngine::workerThread, this);
        }

        Logger::instance().info("RepackEngine started with " +
            std::to_string(m_threadPoolSize) + " worker threads");
    }

    RepackEngine::~RepackEngine() {
        shutdown();
    }

    bool RepackEngine::startJob(RepackJob job) {
        if (!job.validate()) {
            m_lastError = job.getValidationError();
            Logger::instance().error("Invalid job: " + m_lastError);
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_jobQueue.push(std::move(job));
        }
        m_queueCondition.notify_one();

        Logger::instance().info("Job queued: " + job.getConfiguration().gameName);
        return true;
    }

    void RepackEngine::cancelCurrentJob() {
        if (m_currentJob) {
            m_currentJob->cancel();
            m_packageBuilder->cancel();
            Logger::instance().info("Current job cancelled");
        }
    }

    void RepackEngine::cancelAllJobs() {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            while (!m_jobQueue.empty()) {
                m_jobQueue.pop();
            }
        }
        cancelCurrentJob();
        Logger::instance().info("All jobs cancelled");
    }

    bool RepackEngine::isBusy() const {
        return m_currentJob != nullptr || m_activeJobs > 0;
    }

    size_t RepackEngine::getQueueSize() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_jobQueue.size();
    }

    void RepackEngine::shutdown() {
        if (m_shutdownRequested) {
            return;
        }

        m_shutdownRequested = true;
        m_running = false;
        m_queueCondition.notify_all();

        for (auto& thread : m_workerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        m_workerThreads.clear();
        Logger::instance().info("RepackEngine shut down");
    }

    bool RepackEngine::waitForCompletion(uint32_t timeoutMs) {
        std::unique_lock<std::mutex> lock(m_completionMutex);

        if (timeoutMs == 0) {
            m_completionCondition.wait(lock, [this] {
                return m_jobQueue.empty() && m_activeJobs == 0;
                });
            return true;
        }

        return m_completionCondition.wait_for(lock,
            std::chrono::milliseconds(timeoutMs),
            [this] {
                return m_jobQueue.empty() && m_activeJobs == 0;
            });
    }

    void RepackEngine::workerThread() {
        Logger::instance().info("Worker thread started");

        while (m_running) {
            RepackJob job;

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_queueCondition.wait(lock, [this] {
                    return !m_jobQueue.empty() || !m_running;
                    });

                if (!m_running && m_jobQueue.empty()) {
                    break;
                }

                job = std::move(m_jobQueue.front());
                m_jobQueue.pop();
            }

            m_activeJobs++;
            m_processingComplete = false;

            {
                std::lock_guard<std::mutex> lock(m_currentJobMutex);
                m_currentJob = std::make_unique<RepackJob>(std::move(job));
            }

            processJob(*m_currentJob);

            {
                std::lock_guard<std::mutex> lock(m_currentJobMutex);
                m_currentJob.reset();
            }

            m_activeJobs--;
            m_processingComplete = (m_activeJobs == 0 && m_jobQueue.empty());

            m_completionCondition.notify_all();
        }

        Logger::instance().info("Worker thread exiting");
    }

    bool RepackEngine::processJob(RepackJob& job) {
        job.setState(RepackJob::State::Preparing);
        job.setStartTime(std::chrono::steady_clock::now());
        job.notifyProgress(0, "Preparing to repack...");

        auto progressCallback = [&job](int percent, const std::string& status) {
            job.notifyProgress(percent, status);
            };

        auto fileProgressCallback = [&job](const std::string& filename, int percent) {
            // Optional: forward file-level progress
            };

        try {
            job.setState(RepackJob::State::Scanning);
            job.notifyProgress(5, "Scanning source directory...");

            bool result = m_packageBuilder->buildPackage(
                job.getConfiguration().sourceDirectory,
                job.getConfiguration().outputDirectory,
                job.getConfiguration(),
                job.getManifest(),
                progressCallback,
                fileProgressCallback
            );

            if (job.isCancelled()) {
                job.setState(RepackJob::State::Cancelled);
                job.notifyProgress(0, "Cancelled");
                Logger::instance().info("Job cancelled");
                return false;
            }

            if (!result) {
                job.setState(RepackJob::State::Failed);
                job.setLastError(m_packageBuilder->getLastError());
                job.notifyProgress(0, "Failed: " + job.getLastError());
                Logger::instance().error("Job failed: " + job.getLastError());
                return false;
            }

            job.setOriginalSize(m_packageBuilder->getTotalProcessedSize());
            job.setCompressedSize(job.getManifest().calculateTotalCompressedSize());

            auto chunkPaths = m_packageBuilder->getChunkPaths();
            for (const auto& path : chunkPaths) {
                job.addOutputFile(path);
            }

            std::string setupPath = m_packageBuilder->getSetupPath();
            if (!setupPath.empty()) {
                job.addOutputFile(setupPath);
            }

            job.setState(RepackJob::State::Complete);
            job.setEndTime(std::chrono::steady_clock::now());
            job.notifyProgress(100, "Complete!");

            Logger::instance().info("Job completed successfully: " +
                job.getConfiguration().gameName);

            return true;
        }
        catch (const std::exception& e) {
            job.setState(RepackJob::State::Failed);
            job.setLastError(e.what());
            job.notifyProgress(0, "Error: " + std::string(e.what()));
            Logger::instance().error("Job exception: " + std::string(e.what()));
            return false;
        }
    }

    void RepackEngine::cleanup() {
        m_packageBuilder.reset();
    }

} // namespace noty