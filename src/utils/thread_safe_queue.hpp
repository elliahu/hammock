#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

namespace hammock {
    template <typename T>
    class ThreadSafeQueue {
    public:
        void Push(const T& value) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push(value);
            }
            cv_.notify_one(); // Wake up backend if it's sleeping
        }

        // Blocks the backend thread until a command is available
        T Pop() {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !queue_.empty(); });

            T value = queue_.front();
            queue_.pop();
            return value;
        }

    private:
        std::queue<T> queue_;
        std::mutex mutex_;
        std::condition_variable cv_;
    };
}
