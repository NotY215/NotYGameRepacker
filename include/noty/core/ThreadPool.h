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
#include <chrono>
#include <type_traits>

namespace noty {

    class ThreadPool {
    public:
        using Task = std::function<void()>;
        using TaskId = uint64_t;

        struct TaskInfo {
            TaskId id;
            Task task;
            int priority;
            std::chrono::steady_clock::time_point enqueueTime;
        };

        explicit ThreadPool(size_t threadCount = 0);
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        TaskId enqueue(Task task, int priority = 0);

        template<typename F, typename... Args>
        auto enqueueFuture(F&& f, int priority = 0, Args&&... args) 
            -> std::future<typename std::invoke_result_t<F, Args...>>;

        bool cancelTask(TaskId id);
        void cancelAllTasks();
        void waitForAll();
        bool hasPendingTasks() const;
        size_t getPendingTaskCount() const;
        size_t getActiveThreadCount() const;
        size_t getThreadCount() const { return m_threads.size(); }
        void resize(size_t newSize);
        void pause();
        void resume();
        bool isPaused() const { return m_paused.load(std::memory_order_acquire); }
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

        std::atomic<size_t> m_pendingTasks;
        std::atomic<size_t> m_activeThreads;
        size_t m_threadCount;

        std::mutex m_statsMutex;
        std::chrono::steady_clock::time_point m_startTime;
        uint64_t m_tasksCompleted = 0;
        uint64_t m_totalTaskTime = 0;
    };

    // Template implementation
    template<typename F, typename... Args>
    auto ThreadPool::enqueueFuture(F&& f, int priority, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>> 
    {
        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();

        enqueue([task]() { (*task)(); }, priority);

        return result;
    }

} // namespace noty