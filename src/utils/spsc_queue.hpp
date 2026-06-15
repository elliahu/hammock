#pragma once
#include <array>
#include <atomic>
#include <cstddef>

namespace hammock {
    /// @class SPSCQueue
    /// @brief Thread safe queue for single producer single consumer problem
    template <typename T, size_t Capacity>
    class SPSCQueue {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

       public:
        /// @brief Non-blocking push into the queue
        /// @note If the queue is full, silently drops the value and returns false
        bool push(const T& value) {
            const size_t head = head_.load(std::memory_order_relaxed);
            const size_t next = (head + 1) & mask_;

            if (next == tail_.load(std::memory_order_acquire)) return false;  // queue full

            buffer_[head] = value;
            head_.store(next, std::memory_order_release);
            return true;
        }

        /// @brief Non-blocking pop from the queue
        /// @note If the queue is empty, silently fails to asign a value and returns false
        bool pop(T& out) {
            const size_t tail = tail_.load(std::memory_order_relaxed);

            if (tail == head_.load(std::memory_order_acquire)) return false;  // queue empty

            out = buffer_[tail];
            tail_.store((tail + 1) & mask_, std::memory_order_release);
            return true;
        }

       private:
        static constexpr size_t mask_ = Capacity - 1;

        alignas(64) std::atomic<size_t> head_{0};  // producer only writes
        alignas(64) std::atomic<size_t> tail_{0};  // consumer only writes

        std::array<T, Capacity> buffer_;
    };

}  // namespace hammock::threading
