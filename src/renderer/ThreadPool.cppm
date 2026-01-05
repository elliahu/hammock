module;

#include <condition_variable>
#include <mutex>
#include <functional>
#include <queue>
#include <thread>

export module hammock.renderer.threadpool;

namespace hammock::renderer {
    export class ThreadPool {
    public:
        ThreadPool()
            : stop_(false), activeCount_(0) {
        }

        // Sets (or resets) the number of worker threads.
        // If threads already exist, waits for current work, stops them, and spawns new threads.
        void setThreadCount(std::uint32_t threadCount) {
            // Stop any existing worker threads.
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                stop_ = true;
            }
            condition_.notify_all();
            for (std::thread &worker: threads_) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
            threads_.clear();

            // Reset stop flag.
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                stop_ = false;
            }

            // Spawn new worker threads.
            for (std::uint32_t i = 0; i < threadCount; ++i) {
                threads_.emplace_back([this] {
                    while (true) {
                        std::function<void()> job; {
                            std::unique_lock<std::mutex> lock(queueMutex_);
                            condition_.wait(lock, [this] { return stop_ || !jobQueue_.empty(); });
                            // If stopping and no jobs remain, exit the thread loop.
                            if (stop_ && jobQueue_.empty())
                                return;
                            job = std::move(jobQueue_.front());
                            jobQueue_.pop();
                            ++activeCount_; // mark job as in progress
                        }
                        // Execute job outside the lock.
                        job(); {
                            std::lock_guard<std::mutex> lock(queueMutex_);
                            --activeCount_;
                            // Notify waiters if there are no queued or active jobs.
                            if (jobQueue_.empty() && activeCount_ == 0)
                                finishedCondition_.notify_all();
                        }
                    }
                });
            }
        }

        // Submit a new job to the pool.
        void submit(std::function<void()> job) { {
                std::lock_guard<std::mutex> lock(queueMutex_);
                jobQueue_.push(std::move(job));
            }
            condition_.notify_one();
        }

        // Wait until all jobs have been processed.
        void wait() {
            std::unique_lock<std::mutex> lock(queueMutex_);
            finishedCondition_.wait(lock, [this] {
                return jobQueue_.empty() && (activeCount_ == 0);
            });
        }

        // Destructor: stop all threads and join them.
        ~ThreadPool() { {
                std::lock_guard<std::mutex> lock(queueMutex_);
                stop_ = true;
            }
            condition_.notify_all();
            for (std::thread &worker: threads_) {
                if (worker.joinable())
                    worker.join();
            }
        }

    private:
        std::vector<std::thread> threads_;
        std::queue<std::function<void()> > jobQueue_;
        std::mutex queueMutex_;
        std::condition_variable condition_;
        std::condition_variable finishedCondition_;
        std::atomic<int> activeCount_;
        bool stop_;
    };
}
