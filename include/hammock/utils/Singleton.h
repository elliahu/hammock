#pragma once
#include <memory>
#include <stdexcept>

template <typename T>
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    template <typename... Args>
    static void initialize(Args&&... args) {
        if (!instance_) {
            instance_ = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }
    }

    static void dispose() {
        if (instance_ != nullptr) {
            instance_ = nullptr;
        }
    }

    static T& getInstance() {
        if (!instance_) {
            throw std::runtime_error("Singleton not initialized. Call initialize() first.");
        }
        return *instance_;
    }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;

private:
    static inline std::unique_ptr<T> instance_ = nullptr;
};