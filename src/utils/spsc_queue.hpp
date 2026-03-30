#pragma once
#include <array>
#include <atomic>
#include <cstddef>

namespace hammock::threading {
    /// @class SPSCQueue
    /// @brief Thread safe queue for one producer one consumer problem
    /// TODO instead of silently dropping the push when queue is full, pop the front (the oldes) and then push
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

        /// @brief Blocking version of push
        void push_wait(const T& value) {
            size_t head, next;
            while (true) {
                head = head_.load(std::memory_order_relaxed);
                next = (head + 1) & mask_;
                if (next != tail_.load(std::memory_order_acquire)) break;  // has space
                tail_.wait(tail_.load());                                  // wait until tail_ changes
            }
            buffer_[head] = value;
            head_.store(next, std::memory_order_release);
            tail_.notify_one();
        }

        /// Blocking version of pop
        void pop_wait(T& out) {
            size_t tail;
            while (true) {
                tail = tail_.load(std::memory_order_relaxed);
                if (tail != head_.load(std::memory_order_acquire)) break;  // has data
                head_.wait(head_.load());                                  // wait until head_ changes
            }
            out = buffer_[tail];
            tail_.store((tail + 1) & mask_, std::memory_order_release);
            head_.notify_one();
        }

       private:
        static constexpr size_t mask_ = Capacity - 1;

        alignas(64) std::atomic<size_t> head_{0};  // producer only writes
        alignas(64) std::atomic<size_t> tail_{0};  // consumer only writes

        std::array<T, Capacity> buffer_;
    };

}  // namespace hammock::threading
