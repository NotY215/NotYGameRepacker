#include "noty/installer/InstallEngine.h"
#include "noty/installer/ExtractionEngine.h"
#include "noty/common/Logger.h"
#include <thread>
#include <chrono>

namespace noty {

    InstallEngine::InstallEngine()
        : m_running(true)
        , m_shutdownRequested(false)
        , m_threadPoolSize(std::thread::hardware_concurrency())
        , m_extractionBufferSize(1024 * 1024)
        , m_activeJobs(0)
        , m_processingComplete(true)
    {
        if (m_threadPoolSize == 0) {
            m_threadPoolSize = 4; // Fallback
        }

        m_extractionEngine = std::make_unique<ExtractionEngine>();

        // Start worker threads
        for (size_t i = 0; i < m_threadPoolSize; ++i) {
            m_workerThreads.emplace_back(&InstallEngine::workerThread, this);
        }

        Logger::instance().info("InstallEngine started with " +
            std::to_string(m_threadPoolSize) + " worker threads");
    }

    InstallEngine::~InstallEngine() {
        shutdown();
    }

    bool InstallEngine::startJob(InstallJob job) {
        if (!job.validate()) {
            m_lastError = job.getValidationError();
            Logger::instance().error("Invalid job: " + m_lastError);
            return false;
        }

        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_jobQueue.push(std::move(job));
        m_queueCondition.notify_one();

        Logger::instance().info("Install job queued: " + job.getConfiguration().gameName);
        return true;
    }

    void InstallEngine::cancelCurrentJob() {
        if (m_currentJob) {
            m_currentJob->cancel();
            m_extractionEngine->cancel();
            Logger::instance().info("Current install job cancelled");
        }
    }

    void InstallEngine::cancelAllJobs() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_jobQueue.empty()) {
            m_jobQueue.pop();
        }
        cancelCurrentJob();
        Logger::instance().info("All install jobs cancelled");
    }

    bool InstallEngine::isBusy() const {
        return m_currentJob != nullptr || m_activeJobs > 0;
    }

    size_t InstallEngine::getQueueSize() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_jobQueue.size();
    }

    void InstallEngine::shutdown() {
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
        Logger::instance().info("InstallEngine shut down");
    }

    bool InstallEngine::waitForCompletion(uint32_t timeoutMs) {
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

    void InstallEngine::workerThread() {
        Logger::instance().info("Install worker thread started");

        while (m_running) {
            InstallJob job;

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

            // Process the job
            m_activeJobs++;
            m_processingComplete = false;

            {
                std::lock_guard<std::mutex> lock(m_currentJobMutex);
                m_currentJob = std::make_unique<InstallJob>(std::move(job));
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

        Logger::instance().info("Install worker thread exiting");
    }

    bool InstallEngine::processJob(InstallJob& job) {
        job.setState(InstallJob::State::Validating);
        job.setStartTime(std::chrono::steady_clock::now());
        job.notifyProgress(0, "Validating package...");

        // Update progress callback to forward to job
        auto progressCallback = [&job](int percent, const std::string& status) {
            job.notifyProgress(percent, status);
            };

        auto fileProgressCallback = [&job](const std::string& filename, int percent) {
            // Optional: forward file-level progress
            };

        try {
            job.setState(InstallJob::State::Preparing);
            job.notifyProgress(5, "Preparing installation...");

            // Extract the package
            bool result = m_extractionEngine->extractPackage(
                job.getConfiguration().packagePath,
                job.getConfiguration().installDirectory,
                job.getManifest(),
                job.getConfiguration(),
                progressCallback,
                fileProgressCallback
            );

            if (job.isCancelled()) {
                job.setState(InstallJob::State::Cancelled);
                job.notifyProgress(0, "Cancelled");
                Logger::instance().info("Install job cancelled");
                return false;
            }

            if (!result) {
                job.setState(InstallJob::State::Failed);
                job.setLastError(m_extractionEngine->getLastError());
                job.notifyProgress(0, "Failed: " + job.getLastError());
                Logger::instance().error("Install job failed: " + job.getLastError());
                return false;
            }

            // Set job results
            job.setInstalledSize(m_extractionEngine->getExtractedSize());
            job.setExtractedFileCount(m_extractionEngine->getExtractedFileCount());

            // Verify extraction if requested
            if (job.getConfiguration().verifyFiles) {
                job.setState(InstallJob::State::Verifying);
                job.notifyProgress(95, "Verifying installation...");

                if (!m_extractionEngine->verifyExtraction(job.getManifest(),
                    job.getConfiguration().installDirectory)) {
                    job.setLastError("Verification failed");
                    job.notifyProgress(0, "Verification failed");
                    Logger::instance().error("Verification failed");
                    return false;
                }
            }

            job.setState(InstallJob::State::Complete);
            job.setEndTime(std::chrono::steady_clock::now());
            job.notifyProgress(100, "Installation complete!");

            Logger::instance().info("Install job completed successfully: " +
                job.getConfiguration().gameName);

            return true;
        }
        catch (const std::exception& e) {
            job.setState(InstallJob::State::Failed);
            job.setLastError(e.what());
            job.notifyProgress(0, "Error: " + std::string(e.what()));
            Logger::instance().error("Install job exception: " + std::string(e.what()));
            return false;
        }
    }

    void InstallEngine::cleanup() {
        m_extractionEngine.reset();
    }

} // namespace noty