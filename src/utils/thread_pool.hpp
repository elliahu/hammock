#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace hammock::threading {
    class ThreadPool {
       public:
        ThreadPool() : stop_(false), activeCount_(0) {}

        // Sets (or resets) the number of worker threads.
        // If threads already exist, waits for current work, stops them, and spawns new threads.
        void setThreadCount(std::uint32_t threadCount) {
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                stop_ = true;
            }
            condition_.notify_all();
            for (std::thread& worker : threads_) {
                if (worker.joinable()) worker.join();
            }
            threads_.clear();
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                stop_ = false;
            }

            for (std::uint32_t i = 0; i < threadCount; ++i) {
                // Capture i by value so each thread has its own fixed index.
                threads_.emplace_back([this, i] {
                    while (true) {
                        std::function<void(std::uint32_t)> job;
                        {
                            std::unique_lock<std::mutex> lock(queueMutex_);
                            condition_.wait(lock, [this] { return stop_ || !jobQueue_.empty(); });
                            if (stop_ && jobQueue_.empty()) return;
                            job = std::move(jobQueue_.front());
                            jobQueue_.pop();
                            ++activeCount_;
                        }
                        // Pass the thread index into the job.
                        job(i);
                        {
                            std::lock_guard<std::mutex> lock(queueMutex_);
                            --activeCount_;
                            if (jobQueue_.empty() && activeCount_ == 0) finishedCondition_.notify_all();
                        }
                    }
                });
            }
        }

        // Jobs now receive their executing thread's index as a uint32_t argument.
        void submit(std::function<void(std::uint32_t)> job) {
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                jobQueue_.push(std::move(job));
            }
            condition_.notify_one();
        }

        void wait() {
            std::unique_lock<std::mutex> lock(queueMutex_);
            finishedCondition_.wait(lock, [this] { return jobQueue_.empty() && (activeCount_ == 0); });
        }

        ~ThreadPool() {
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                stop_ = true;
            }
            condition_.notify_all();
            for (std::thread& worker : threads_) {
                if (worker.joinable()) worker.join();
            }
        }

       private:
        std::vector<std::thread> threads_;
        std::queue<std::function<void(std::uint32_t)>> jobQueue_;  // <-- updated type
        std::mutex queueMutex_;
        std::condition_variable condition_;
        std::condition_variable finishedCondition_;
        std::atomic<int> activeCount_;
        bool stop_;
    };
}  // namespace hammock::threading
