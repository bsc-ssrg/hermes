#ifndef __LOGGING_HPP__
#define __LOGGING_HPP__

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

template <typename Derived, typename Base>
class cloneable : public Base {

public:

    virtual
    ~cloneable() = default;

    std::unique_ptr<Derived>
    clone() const {
        return std::unique_ptr<Derived>(
                static_cast<Derived*>(this->clone_impl()));
    }

private:
    virtual cloneable*
    clone_impl() const override final {
        return new Derived(static_cast<const Derived&>(*this));
    }
};

namespace log {

    template <typename Derived, typename Base>
    using logger_interface = cloneable<Derived, Base>;

    static int mercury_log_hook(FILE* stream, const char* fmt, ...);

    /*! abstract interface for pluggable loggers  */
    struct logger {

        logger() {
            detail::set_mercury_log_function(mercury_log_hook);
        }

        virtual ~logger() = default;

        template <typename... Args>
        static std::string
        format(const char* fmt, Args&&... args) {
            return fmt::format(fmt, std::forward<Args>(args)...);
        }

        virtual void
        info(const std::string& msg) const = 0;

        virtual void
        warning(const std::string& msg) const = 0;

        virtual void
        error(const std::string& msg) const = 0;
        
        virtual void
        fatal(const std::string& msg) const = 0;
        
#ifdef HERMES_DEBUG_BUILD
        virtual void
        debug(int level, const char* file, const char* func, 
              int line, const std::string& msg) const = 0;
#endif
        
        virtual int
        mercury_log(std::FILE* stream, const char* fmt, ::va_list ap) const = 0;

        std::unique_ptr<logger> clone() const {
            return std::unique_ptr<logger>(this->clone_impl());
        }

private:
        virtual logger* clone_impl() const = 0;

    };

    /* no op logger */
    struct nop_logger : public logger_interface<nop_logger, logger> {

        void 
        info(const std::string&) const override final { }

        void 
        error(const std::string&) const override final { }

        void 
        warning(const std::string&) const override final { }

        void 
        fatal(const std::string&) const override final { }

#ifdef HERMES_DEBUG_BUILD
        void 
        debug(int, const char*, const char*, int, 
              const std::string&) const override final { }
#endif

        int
        mercury_log(FILE*, const char*, va_list) const override final { 
            return 0;
        }

    };

    static inline std::shared_ptr<logger>&
    shared_logger() {
        static std::shared_ptr<logger> _ = std::make_shared<nop_logger>();
        return _;
    }

    static inline logger&
    register_shared_logger(const logger& logger) {
        shared_logger() = logger.clone();
        return *shared_logger();
    }

    static inline logger&
    get_shared_logger() {
        return *shared_logger();
    }

    static int mercury_log_hook(FILE* stream, const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        const int rv = shared_logger()->mercury_log(stream, fmt, ap);
        va_end(ap);

        return rv;
    }

} // namespace log
} // namespace hermes

#define HERMES_REGISTER_LOGGER(type)                     \
    namespace {                                          \
        const static auto& _ =                           \
            hermes::log::register_shared_logger(type{}); \
    }

#define HERMES_INFO(...) \
    do { \
        using namespace hermes::log; \
        get_shared_logger().info(logger::format(__VA_ARGS__)); \
    } while(0);


#define HERMES_WARNING(...) \
    do { \
        using namespace hermes::log; \
        get_shared_logger().warning(logger::format(__VA_ARGS__)); \
    } while(0);

#define HERMES_ERROR(...) \
    do { \
        using namespace hermes::log; \
        get_shared_logger().error(logger::format(__VA_ARGS__)); \
    } while(0);


#define HERMES_FATAL(...) \
    do { \
        using namespace hermes::log; \
        get_shared_logger().fatal(logger::format(__VA_ARGS__)); \
    } while(0);

#ifdef HERMES_DEBUG_BUILD

#define HERMES_DEBUG(...) \
    HERMES_DEBUG_HELPER(1u, __VA_ARGS__);

#define HERMES_DEBUG2(...) \
    HERMES_DEBUG_HELPER(2u, __VA_ARGS__);


#define HERMES_DEBUG3(...) \
    HERMES_DEBUG_HELPER(3u, __VA_ARGS__);

#define HERMES_DEBUG4(...) \
    HERMES_DEBUG_HELPER(4u, __VA_ARGS__);

#define HERMES_DEBUG_HELPER(level, ...)                                       \
    do {                                                                      \
        __typeof(level) l = (level);                                          \
        hermes::log::get_shared_logger().debug(l, __FILE__, __func__,         \
                __LINE__, log::logger::format(__VA_ARGS__));  \
    } while(0);

#define HERMES_DEBUG_FLUSH()    \
    do {                        \
        ::flush(stdout);        \
    } while(0);

#else // ! HERMES_DEBUG_BUILD

#define HERMES_DEBUG(...)       \
    do { } while(0); 

#define HERMES_DEBUG2(...)     \
    HERMES_DEBUG(__VA_ARGS__)

#define HERMES_DEBUG3(...)      \
    HERMES_DEBUG(__VA_ARGS__)

#define HERMES_DEBUG4(...)      \
    HERMES_DEBUG(__VA_ARGS__)

#define HERMES_DEBUG_FLUSH()    \
    do { } while(0);

#endif // HERMES_DEBUG_BUILD

#endif // __LOGGING_HPP__
