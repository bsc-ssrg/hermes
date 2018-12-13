#ifndef __LOGGING_HPP__
#define __LOGGING_HPP__

#ifdef HAVE_FMT

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <thread>

namespace logging {
static unsigned int debug_output_level = 1;

void
set_debug_output_level(unsigned int level) {
    debug_output_level = level;
}
} // namespace logging

#define HERMES_INFO(...) \
    do { \
        fmt::print(stdout, "INFO: {}\n", fmt::format(__VA_ARGS__)); \
    } while(0);


#define HERMES_WARNING(...) \
    do { \
        fmt::print(stdout, "WARNING: {}\n", fmt::format(__VA_ARGS__)); \
    } while(0);


#ifdef HERMES_DEBUG_BUILD

#define HERMES_DEBUG(...) \
    DEBUG_HELPER(1u, __VA_ARGS__);


#define HERMES_DEBUG2(...) \
    DEBUG_HELPER(2u, __VA_ARGS__);


#define HERMES_DEBUG3(...) \
    DEBUG_HELPER(3u, __VA_ARGS__);

#define HERMES_DEBUG4(...) \
    DEBUG_HELPER(4u, __VA_ARGS__);

#define DEBUG_HELPER(level, ...)                                            \
    do {                                                                    \
        __typeof(level) l = (level);                                        \
        if(logging::debug_output_level >= l) {                              \
            fmt::print(stdout, "[{}:{}()] [{}] {} {}\n", __FILE__,  __func__,    \
                       std::this_thread::get_id(), "DEBUG:",                \
                       fmt::format(__VA_ARGS__));                           \
        }                                                                   \
    } while(0);

#else // ! HERMES_DEBUG

#define HERMES_DEBUG(...)  \
    do {            \
    } while(0); 


#define HERMES_DEBUG2(...)     \
    HERMES_DEBUG(__VA_ARGS__)


#define HERMES_DEBUG3(...)     \
    HERMES_DEBUG(__VA_ARGS__)

#define HERMES_DEBUG4(...)     \
    HERMES_DEBUG(__VA_ARGS__)

#endif

    
#define ERROR(...) \
    do { \
        fmt::print(stderr, "ERROR: {}\n", fmt::format(__VA_ARGS__)); \
    } while(0);


#define FATAL(...) \
    do { \
        fmt::print(stderr, "FATAL: {}\n", fmt::format(__VA_ARGS__)); \
        exit(1); \
    } while(0);


#else // ! HAVE_FMT

#define HERMES_INFO(...)
#define HERMES_WARNING(...)
#define HERMES_DEBUG(...)
#define HERMES_DEBUG2(...)
#define HERMES_DEBUG3(...)
#define HERMES_DEBUG4(...)
#define ERROR(...)
#define FATAL(...)

#endif // HAVE_FMT

#endif // __LOGGING_HPP__
