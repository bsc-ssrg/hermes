#ifndef HERMES_LOGGING_HPP
#define HERMES_LOGGING_HPP

/* noop macros for logging */
#define HERMES_INFO(...) do { } while(0); 
#define HERMES_WARNING(...) do { } while(0);
#define HERMES_ERROR(...) do { } while(0);
#define HERMES_FATAL(...) do { } while(0);
#define HERMES_DEBUG(...) do { } while(0); 
#define HERMES_DEBUG2(...) HERMES_DEBUG(__VA_ARGS__)
#define HERMES_DEBUG3(...) HERMES_DEBUG(__VA_ARGS__)
#define HERMES_DEBUG4(...) HERMES_DEBUG(__VA_ARGS__)

#ifdef HERMES_ENABLE_LOGGING

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <memory>
#include <stdarg.h>

namespace hermes {

namespace detail {

// forward declare detail::set_mercury_log_function()
using mercury_log_fuction = int(FILE *stream, const char *format, ...);
void set_mercury_log_function(mercury_log_fuction);

} // namespace detail

namespace log {

enum class level : int {
    info = 0,
    warning,
    error,
    fatal,
#ifdef HERMES_DEBUG_BUILD
    debug,
#endif
    mercury,
    max
};

static constexpr auto info = level::info;
static constexpr auto warning = level::warning;
static constexpr auto error = level::error;
static constexpr auto fatal = level::fatal;

#ifdef HERMES_DEBUG_BUILD
static constexpr auto debug = level::debug;
#endif

static constexpr auto mercury = level::mercury;

struct logger;

using log_callback = 
    std::function<
        void(const std::string&,  // message
             log::level,          // level
             int,                 // severity
             const std::string&,  // file name
             const std::string&,  // function name
             int                  // line number
    )>;

class logger {

    enum { mercury_message_size = 1024 };

    static int mercury_log_hook(FILE* stream, const char* fmt, ...) {

        (void) stream;

        char buffer[mercury_message_size];

        va_list ap;
        va_start(ap, fmt);
        const int n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
        get().mercury_log(buffer);
        va_end(ap);

        return n;
    }

    struct noop {
        struct anything {
            template <class T>
            operator T() { return {}; }
        };

        template<class...Args>
        anything operator()(Args&&...) const {
            return {};
        }
    };

public:
    logger() :
        m_log_callbacks({
                noop{}, noop{}, noop{}, noop{}, 
#ifdef HERMES_DEBUG_BUILD
                noop{}, 
#endif
                noop{}}) {

        detail::set_mercury_log_function(mercury_log_hook);
    }

    static logger&
    get() {
        static logger _;
        return _;
    }

    template <typename Callback>
    static void
    register_callback(level l, Callback&& cb) {
        get().add_callback(l, std::forward<Callback>(cb));
    }

public:

    template <typename... Args>
    static std::string
    format(const char* fmt, Args&&... args) {
        return fmt::format(fmt, std::forward<Args>(args)...);
    }

    void
    info(const std::string& msg) const { 
        const int l = static_cast<int>(level::info);
        m_log_callbacks.at(l)(msg, level::info, 0, "", "", -1);
    }

    void
    warning(const std::string& msg) const { 
        const int l = static_cast<int>(level::warning);
        m_log_callbacks.at(l)(msg, level::warning, 0, "", "", -1);
    }

    void
    error(const std::string& msg) const { 
        const int l = static_cast<int>(level::error);
        m_log_callbacks.at(l)(msg, level::error, 0, "", "", -1);
    }
    
    void
    fatal(const std::string& msg) const { 
        const int l = static_cast<int>(level::fatal);
        m_log_callbacks.at(l)(msg, level::fatal, 0, "", "", -1);
    }
    
#ifdef HERMES_DEBUG_BUILD
    void
    debug(int level, const char* file, const char* func, 
          int line, const std::string& msg) const {
        const int l = static_cast<int>(level::debug);
        m_log_callbacks.at(l)(msg, level::debug, level, file, func, line);
    }
#endif
    
    void
    mercury_log(const std::string& msg) const { 
        const int l = static_cast<int>(level::mercury);
        m_log_callbacks.at(l)(msg, level::mercury, 0, "", "", -1);
    }

private:

    template <typename Callback>
    void
    add_callback(level l, Callback&& cb) {
        m_log_callbacks[static_cast<int>(l)] = std::forward<Callback>(cb);
    }

private:
    using callback_set = 
        std::array<log_callback, static_cast<int>(level::max)>;

    callback_set m_log_callbacks;
};

} // namespace log
} // namespace hermes

#undef HERMES_INFO
#define HERMES_INFO(...) \
    do { \
        using namespace hermes::log; \
        logger::get().info(logger::format(__VA_ARGS__)); \
    } while(0);


#undef HERMES_WARNING
#define HERMES_WARNING(...) \
    do { \
        using namespace hermes::log; \
        logger::get().warning(logger::format(__VA_ARGS__)); \
    } while(0);

#undef HERMES_ERROR
#define HERMES_ERROR(...) \
    do { \
        using namespace hermes::log; \
        logger::get().error(logger::format(__VA_ARGS__)); \
    } while(0);

#undef HERMES_FATAL
#define HERMES_FATAL(...) \
    do { \
        using namespace hermes::log; \
        logger::get().fatal(logger::format(__VA_ARGS__)); \
    } while(0);

#ifdef HERMES_DEBUG_BUILD

#define HERMES_DEBUG_HELPER(level, ...)                 \
    do {                                                \
        using namespace hermes::log;                    \
        __typeof(level) l = (level);                    \
        logger::get().debug(                            \
                l, __FILE__, __func__,                  \
                __LINE__, logger::format(__VA_ARGS__)); \
    } while(0);

#undef HERMES_DEBUG
#define HERMES_DEBUG(...) HERMES_DEBUG_HELPER(0u, __VA_ARGS__);

#undef HERMES_DEBUG2
#define HERMES_DEBUG2(...) HERMES_DEBUG_HELPER(1u, __VA_ARGS__);

#undef HERMES_DEBUG3
#define HERMES_DEBUG3(...) HERMES_DEBUG_HELPER(2u, __VA_ARGS__);

#undef HERMES_DEBUG4
#define HERMES_DEBUG4(...) HERMES_DEBUG_HELPER(3u, __VA_ARGS__);

#endif // HERMES_DEBUG_BUILD

#endif // HERMES_ENABLE_LOGGING

#endif // HERMES_LOGGING_HPP
