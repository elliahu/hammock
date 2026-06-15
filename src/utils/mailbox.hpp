#pragma once
#include <atomic>

namespace hammock {
    template <typename T>
    class Mailbox {
    public:
        void push(const T& value) {
            write_.store(value, std::memory_order_relaxed);
            dirty_.store(true, std::memory_order_release);
        }

        void swap() {
            if (dirty_.exchange(false, std::memory_order_acq_rel)) {
                read_ = write_.load(std::memory_order_acquire);
            }
        }

        const T& current() const { return read_; }

    private:
        std::atomic<T> write_;
        T read_{};
        std::atomic<bool> dirty_{false};
    };
}
