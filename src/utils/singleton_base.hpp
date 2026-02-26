#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>

namespace hammock::helpers {
    template <typename T>
    class SingletonBase {
       public:
        // Access singleton instance
        static inline T& instance() {
            std::call_once(initFlag_, &SingletonBase::Init);
            assert(instance_ && "Singleton accessed before initialization");
            return *instance_;
        }

        // Initialize singleton with constructor arguments
        template <typename... Args>
        static inline void initialize(Args&&... args) {
            std::call_once(
                initFlag_, [&] { instance_ = std::unique_ptr<T>(new T(std::forward<Args>(args)...)); });
        }

        static inline void shutdown() { instance_.reset(); }

        // Delete copy/move
        SingletonBase(const SingletonBase&) = delete;
        SingletonBase& operator=(const SingletonBase&) = delete;

       protected:
        SingletonBase() = default;
        ~SingletonBase() = default;

       private:
        static inline void Init() { assert(instance_ && "Singleton not initialized before first use"); }

        static inline std::unique_ptr<T> instance_;
        static inline std::once_flag initFlag_;
    };
}  // namespace hammock::helpers