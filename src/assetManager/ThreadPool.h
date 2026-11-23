#pragma once

#include <array>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

enum class TaskPriority : uint8_t {
    High = 0,
    Normal = 1,
    Low = 2
};

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = std::max(1u, std::thread::hardware_concurrency() - 1)) {
        workers.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            workers.emplace_back([this] { workerLoop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stopping = true;
        }
        cv.notify_all();
        for (auto& w : workers) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F>
    void enqueue(F&& job, TaskPriority priority = TaskPriority::Normal) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stopping) {
                return;
            }
            const auto idx = static_cast<size_t>(priority);
            queues[idx].emplace(std::forward<F>(job));
        }
        cv.notify_one();
    }

    void clearLowPriority() {
        std::lock_guard<std::mutex> lock(mtx);
        std::queue<std::function<void()>> empty;
        std::swap(queues[static_cast<size_t>(TaskPriority::Low)], empty);
    }

private:
    bool hasPendingJobs() const {
        for (const auto& q : queues) {
            if (!q.empty()) {
                return true;
            }
        }
        return false;
    }

    void workerLoop() {
        while (true) {
            std::function<void()> job;

            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return stopping || hasPendingJobs(); });

                if (stopping && !hasPendingJobs()) {
                    return;
                }

                for (auto& q : queues) {
                    if (!q.empty()) {
                        job = std::move(q.front());
                        q.pop();
                        break;
                    }
                }
            }

            job();
        }
    }

    std::vector<std::thread> workers;
    std::array<std::queue<std::function<void()>>, 3> queues;
    std::mutex mtx;
    std::condition_variable cv;
    bool stopping = false;
};