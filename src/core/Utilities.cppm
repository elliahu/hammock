module;

#include <functional>
#include <memory>
#include <stdexcept>

export module hammock.core.utilities;



namespace hammock::core {

    export template<typename T>
    class Singleton {
    public:
        Singleton(const Singleton &) = delete;

        Singleton &operator=(const Singleton &) = delete;

        template<typename... Args>
        static void initialize(Args &&... args) {
            if (!instance_) {
                instance_ = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
            }
        }

        static void dispose() {
            if (instance_) {
                instance_.reset();
            }
        }

        static T &getInstance() {
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


    export enum LogLevel {
        LOG_LEVEL_NONE = 0, // Added NONE for stricter control
        LOG_LEVEL_ERROR,
        LOG_LEVEL_WARN,
        LOG_LEVEL_INFO,
        LOG_LEVEL_DEBUG,
    };

    /// Static logging class providing level-specific wrappers
    export class Logger {
    public:
        // Global minimum log level.
#ifdef NDEBUG
    // In Release builds, set minimum level to WARNING or ERROR
    static inline LogLevel hmckMinLogLevel = LOG_LEVEL_WARN;
#else
        // In Debug builds, set minimum level to DEBUG
        static inline LogLevel hmckMinLogLevel = LOG_LEVEL_DEBUG;
#endif

        // --- Public Wrapper Functions ---

        /**
         * @brief New public wrapper for INFO level messages.
         *
         * Usage: Logger::info("msg %d", 2);
         *
         * @param format The format string (like in printf).
         * @param args Variadic arguments to be formatted.
         */
        template<typename... Args>
        static void info(const char *format, Args &&... args) {
            if (LOG_LEVEL_INFO >= hmckMinLogLevel) {
                log_impl(LOG_LEVEL_INFO, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief New public wrapper for DEBUG level messages.
         *
         * Usage: Logger::debug("msg %d", 2);
         *
         * @param format The format string (like in printf).
         * @param args Variadic arguments to be formatted.
         */
        template<typename... Args>
        static void debug(const char *format, Args &&... args) {
            if (LOG_LEVEL_DEBUG >= hmckMinLogLevel) {
                log_impl(LOG_LEVEL_DEBUG, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief New public wrapper for WARN level messages.
         *
         * Usage: Logger::warn("msg %s", "issue");
         */
        template<typename... Args>
        static void warn(const char *format, Args &&... args) {
            if (LOG_LEVEL_WARN >= hmckMinLogLevel) {
                log_impl(LOG_LEVEL_WARN, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief New public wrapper for ERROR level messages.
         *
         * Usage: Logger::error("Failed: %d", -1);
         */
        template<typename... Args>
        static void error(const char *format, Args &&... args) {
            if (LOG_LEVEL_ERROR >= hmckMinLogLevel) {
                log_impl(LOG_LEVEL_ERROR, format, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Original general log function.
         *
         * This maintains the original signature for compatibility:
         * Logger::log(LOG_LEVEL_DEBUG, "msg %d", 2);
         */
        template<typename... Args>
        static void log(const LogLevel level, const char *format, Args &&... args) {
            if (level >= hmckMinLogLevel) {
                log_impl(level, format, std::forward<Args>(args)...);
            }
        }

    private:
        /**
         * @brief Core internal logging function.
         *
         * All public logging functions call this function. It contains the actual
         * prefix generation and output logic.
         *
         * @param level The severity level of the log message.
         * @param format The format string.
         * @param args The variadic arguments.
         */
        template<typename... Args>
        static void log_impl(const LogLevel level, const char *format, Args &&... args) {
            const char *prefix = "";

            // Determine the prefix based on the log level
            switch (level) {
                case LOG_LEVEL_DEBUG:
                    prefix = "DEBUG: ";
                    break;
                case LOG_LEVEL_INFO:
                    prefix = "INFO: ";
                    break;
                case LOG_LEVEL_WARN:
                    prefix = "WARNING: ";
                    break;
                case LOG_LEVEL_ERROR:
                    prefix = "ERROR: ";
                    break;
                default:
                    // Do nothing for NONE or unrecognized
                    return;
            }

            // Print the prefix first
            std::printf("%s", prefix);

            // Print the formatted message using printf with forwarded arguments
            std::printf(format, std::forward<Args>(args)...);

            // Add a newline for clean output
            std::printf("\n");
        }
    };

    export class AutoDispose {
    public:
        using Deleter = std::function<void(const void*)>;

        explicit AutoDispose(const void* ptr = nullptr, Deleter deleter = nullptr)
            : memory_(ptr), deleter_(std::move(deleter)) {}

        ~AutoDispose() { clear(); }

        AutoDispose(AutoDispose&& other) noexcept
            : memory_(other.memory_), deleter_(std::move(other.deleter_)) {
            other.memory_ = nullptr;
        }

        AutoDispose& operator=(AutoDispose&& other) noexcept {
            if (this != &other) {
                clear();
                memory_ = other.memory_;
                deleter_ = std::move(other.deleter_);
                other.memory_ = nullptr;
            }
            return *this;
        }

        AutoDispose(const AutoDispose&) = delete;
        AutoDispose& operator=(const AutoDispose&) = delete;

        void clear() {
            if (memory_) {
                if (deleter_) deleter_(memory_);
                memory_ = nullptr;
            }
        }

        [[nodiscard]] const void* get() const { return memory_; }

    private:
        const void* memory_;
        Deleter deleter_;
    };

    export class Scoped {
    public:
        explicit Scoped(std::function<void()> onExitScope)
            : callback_(std::move(onExitScope)), active_(true) {}

        // Move constructor
        Scoped(Scoped&& other) noexcept : callback_(std::move(other.callback_)), active_(other.active_) {
            other.active_ = false;
        }

        // Disable copy
        Scoped(const Scoped&) = delete;
        Scoped& operator=(const Scoped&) = delete;

        // Dismiss the callback if needed
        void dismiss() { active_ = false; }

        ~Scoped() {
            if (active_ && callback_) {
                callback_();
            }
        }

    private:
        std::function<void()> callback_;
        bool active_;
    };

    // Helper function to simplify usage
    export template <typename F>
    Scoped makeScoped(F&& f) {
        return Scoped(std::forward<F>(f));
    }

    export inline std::uint32_t getNumberOfMipLevels(const std::uint32_t width, const std::uint32_t height) {
        return static_cast<std::uint32_t>(std::floor(std::log2((std::min)(width, height)))) + 1;
    }

    export inline size_t alignSize(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }
}
