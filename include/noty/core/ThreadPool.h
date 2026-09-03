================================================================================
FILE: include / noty / core / ThreadPool.h
================================================================================
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace noty {

    /**
     * @brief Thread pool for parallel processing
     *
     * Provides a thread pool with work stealing and adaptive sizing.
     * Supports task prioritization and cancellation.
     */
    class ThreadPool {
    public:
        using Task = std::function<void()>;
        using TaskId = uint64_t;

        struct TaskInfo {
            TaskId id;
            Task task;
            int priority;  // Higher = more important
            std::chrono::steady_clock::time_point enqueueTime;
        };

        explicit ThreadPool(size_t threadCount = 0);
        ~ThreadPool();

        // Non-copyable
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        /**
         * @brief Enqueue a task with optional priority
         * @param task The task to execute
         * @param priority Task priority (higher = executed first)
         * @return Task ID for tracking
         */
        TaskId enqueue(Task task, int priority = 0);

        /**
         * @brief Enqueue a task and get a future for the result
         * @param task The task to execute
         * @param priority Task priority (higher = executed first)
         * @return Future for the task result
         */
        template<typename F, typename... Args>
        auto enqueueFuture(F&& f, int priority = 0, Args&&... args)
            -> std::future<typename std::result_of<F(Args...)>::type>;

        /**
         * @brief Cancel a specific task by ID
         * @param id Task ID to cancel
         * @return true if task was found and cancelled
         */
        bool cancelTask(TaskId id);

        /**
         * @brief Cancel all pending tasks
         */
        void cancelAllTasks();

        /**
         * @brief Wait for all tasks to complete
         */
        void waitForAll();

        /**
         * @brief Check if there are pending tasks
         */
        bool hasPendingTasks() const;

        /**
         * @brief Get the number of pending tasks
         */
        size_t getPendingTaskCount() const;

        /**
         * @brief Get the number of active worker threads
         */
        size_t getActiveThreadCount() const;

        /**
         * @brief Get the total thread count
         */
        size_t getThreadCount() const { return m_threads.size(); }

        /**
         * @brief Resize the thread pool
         * @param newSize New thread count (0 = auto-detect)
         */
        void resize(size_t newSize);

        /**
         * @brief Pause task processing
         */
        void pause();

        /**
         * @brief Resume task processing
         */
        void resume();

        /**
         * @brief Check if pool is paused
         */
        bool isPaused() const { return m_paused.load(std::memory_order_acquire); }

        /**
         * @brief Shutdown the thread pool
         */
        void shutdown();

    private:
        void workerThread();
        TaskInfo getNextTask();
        void processTask(TaskInfo& info);

        std::vector<std::thread> m_threads;
        std::vector<TaskInfo> m_taskQueue;
        mutable std::mutex m_queueMutex;
        std::condition_variable m_condition;
        std::atomic<bool> m_running;
        std::atomic<bool> m_paused;
        std::atomic<TaskId> m_nextTaskId;
        std::atomic<bool> m_shutdownRequested;

        // Statistics
        std::atomic<size_t> m_pendingTasks;
        std::atomic<size_t> m_activeThreads;
        size_t m_threadCount;

        // Performance tracking
        std::mutex m_statsMutex;
        std::chrono::steady_clock::time_point m_startTime;
        uint64_t m_tasksCompleted = 0;
        uint64_t m_totalTaskTime = 0;
    };

    // Template implementation
    template<typename F, typename... Args>
    auto ThreadPool::enqueueFuture(F&& f, int priority, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();

        enqueue([task]() { (*task)(); }, priority);

        return result;
    }

} // namespace noty