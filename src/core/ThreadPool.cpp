#include "noty/core/ThreadPool.h"
#include "noty/common/Logger.h"
#include <algorithm>
#include <chrono>

namespace noty {

    ThreadPool::ThreadPool(size_t threadCount)
        : m_running(true)
        , m_paused(false)
        , m_nextTaskId(0)
        , m_shutdownRequested(false)
        , m_pendingTasks(0)
        , m_activeThreads(0)
        , m_threadCount(threadCount)
        , m_tasksCompleted(0)
        , m_totalTaskTime(0)
    {
        if (m_threadCount == 0) {
            m_threadCount = std::thread::hardware_concurrency();
            if (m_threadCount == 0) m_threadCount = 4;
        }

        m_startTime = std::chrono::steady_clock::now();

        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads.emplace_back(&ThreadPool::workerThread, this);
        }

        Logger::instance().info("ThreadPool created with " +
            std::to_string(m_threadCount) + " threads");
    }

    ThreadPool::~ThreadPool() {
        shutdown();
    }

    ThreadPool::TaskId ThreadPool::enqueue(Task task, int priority) {
        if (m_shutdownRequested) {
            return 0;
        }

        TaskId id = m_nextTaskId.fetch_add(1, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_taskQueue.push_back({ id, std::move(task), priority,
                std::chrono::steady_clock::now() });

            // Sort by priority (higher first)
            std::sort(m_taskQueue.begin(), m_taskQueue.end(),
                [](const TaskInfo& a, const TaskInfo& b) {
                    return a.priority > b.priority;
                });
        }

        m_pendingTasks.fetch_add(1, std::memory_order_release);
        m_condition.notify_one();

        return id;
    }

    bool ThreadPool::cancelTask(TaskId id) {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        auto it = std::find_if(m_taskQueue.begin(), m_taskQueue.end(),
            [id](const TaskInfo& info) { return info.id == id; });

        if (it != m_taskQueue.end()) {
            m_taskQueue.erase(it);
            m_pendingTasks.fetch_sub(1, std::memory_order_release);
            return true;
        }

        return false;
    }

    void ThreadPool::cancelAllTasks() {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        size_t count = m_taskQueue.size();
        m_taskQueue.clear();
        m_pendingTasks.fetch_sub(count, std::memory_order_release);
        Logger::instance().info("Cancelled " + std::to_string(count) + " pending tasks");
    }

    void ThreadPool::waitForAll() {
        while (hasPendingTasks() || m_activeThreads > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    bool ThreadPool::hasPendingTasks() const {
        return m_pendingTasks.load(std::memory_order_acquire) > 0;
    }

    size_t ThreadPool::getPendingTaskCount() const {
        return m_pendingTasks.load(std::memory_order_acquire);
    }

    size_t ThreadPool::getActiveThreadCount() const {
        return m_activeThreads.load(std::memory_order_acquire);
    }

    void ThreadPool::resize(size_t newSize) {
        if (newSize == 0) {
            newSize = std::thread::hardware_concurrency();
            if (newSize == 0) newSize = 4;
        }

        if (newSize == m_threads.size()) {
            return;
        }

        Logger::instance().info("Resizing thread pool from " +
            std::to_string(m_threads.size()) + " to " + std::to_string(newSize) + " threads");

        // Wait for all current tasks to complete
        waitForAll();

        // Shutdown existing threads
        m_running = false;
        m_condition.notify_all();

        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        m_threads.clear();

        // Create new threads
        m_running = true;
        m_threadCount = newSize;

        for (size_t i = 0; i < m_threadCount; ++i) {
            m_threads.emplace_back(&ThreadPool::workerThread, this);
        }

        Logger::instance().info("Thread pool resized to " +
            std::to_string(m_threadCount) + " threads");
    }

    void ThreadPool::pause() {
        m_paused.store(true, std::memory_order_release);
        Logger::instance().info("Thread pool paused");
    }

    void ThreadPool::resume() {
        m_paused.store(false, std::memory_order_release);
        m_condition.notify_all();
        Logger::instance().info("Thread pool resumed");
    }

    void ThreadPool::shutdown() {
        if (m_shutdownRequested) {
            return;
        }

        m_shutdownRequested = true;
        m_running = false;
        m_condition.notify_all();

        for (auto& thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        m_threads.clear();

        Logger::instance().info("Thread pool shut down. Completed " +
            std::to_string(m_tasksCompleted) + " tasks");
    }

    void ThreadPool::workerThread() {
        while (m_running || hasPendingTasks()) {
            // Check if paused
            if (m_paused.load(std::memory_order_acquire)) {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_condition.wait(lock, [this] {
                    return !m_paused.load(std::memory_order_acquire) || !m_running;
                    });
                if (!m_running) break;
            }

            TaskInfo task = getNextTask();
            if (task.id == 0) {
                // No task available
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_condition.wait_for(lock, std::chrono::milliseconds(100), [this] {
                    return !m_taskQueue.empty() || !m_running;
                    });
                continue;
            }

            processTask(task);
        }
    }

    ThreadPool::TaskInfo ThreadPool::getNextTask() {
        std::lock_guard<std::mutex> lock(m_queueMutex);

        if (m_taskQueue.empty()) {
            return { 0, nullptr, 0, {} };
        }

        TaskInfo task = std::move(m_taskQueue.front());
        m_taskQueue.erase(m_taskQueue.begin());
        m_pendingTasks.fetch_sub(1, std::memory_order_release);

        return task;
    }

    void ThreadPool::processTask(TaskInfo& info) {
        m_activeThreads.fetch_add(1, std::memory_order_release);

        auto startTime = std::chrono::steady_clock::now();

        try {
            if (info.task) {
                info.task();
            }
        }
        catch (const std::exception& e) {
            Logger::instance().error("Task " + std::to_string(info.id) +
                " threw exception: " + e.what());
        }
        catch (...) {
            Logger::instance().error("Task " + std::to_string(info.id) +
                " threw unknown exception");
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime).count();

        m_activeThreads.fetch_sub(1, std::memory_order_release);
        m_tasksCompleted++;
        m_totalTaskTime += duration;
    }

} // namespace noty