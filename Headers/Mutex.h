#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template <typename T>
class MutexQueue {
private:
    std::queue<std::unique_ptr<T>> queue;
    std::mutex mtx;
    std::condition_variable cv;

public:
    MutexQueue() = default;

    // No copying allowed
    MutexQueue(const MutexQueue&) = delete;
    MutexQueue& operator=(const MutexQueue&) = delete;

    // Add element in queue
    void Push(std::unique_ptr<T> item) {
        if (!item) return;
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(std::move(item));
        cv.notify_one(); // Notify GPU thread
    }

    // Extract an element if it exists (called from C++ Main Thread, OpenGL & GPU heavy)
    std::unique_ptr<T> TryPop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) {
            return nullptr;
        }

        std::unique_ptr<T> value = std::move(queue.front());
        queue.pop();
        return value;
    }

    // Block thread until the queue is no longer empty (for brute sincronization)
    std::unique_ptr<T> WaitAndPop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty(); });

        std::unique_ptr<T> value = std::move(queue.front());
        queue.pop();
        return value;
    }

    // Empty the queue completely
    bool Empty() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }
};
