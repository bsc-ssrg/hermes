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

#define INFO(...) \
    do { \
        fmt::print(stdout, "INFO: {}\n", fmt::format(__VA_ARGS__)); \
    } while(0);


#define WARNING(...) \
    do { \
        fmt::print(stdout, "WARNING: {}\n", fmt::format(__VA_ARGS__)); \
    } while(0);


#ifdef HERMES_DEBUG

#define DEBUG(...) \
    DEBUG_HELPER(1u, __VA_ARGS__);


#define DEBUG2(...) \
    DEBUG_HELPER(2u, __VA_ARGS__);


#define DEBUG3(...) \
    DEBUG_HELPER(3u, __VA_ARGS__);

#define DEBUG4(...) \
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

#define DEBUG(...)  \
    do {            \
    } while(0); 


#define DEBUG2(...)     \
    DEBUG(__VA_ARGS__)


#define DEBUG3(...)     \
    DEBUG(__VA_ARGS__)

#define DEBUG4(...)     \
    DEBUG(__VA_ARGS__)

#endif

    
#define ERROR(...) \
    do { \
        fmt::print(stderr, "ERROR: " __VA_ARGS__); \
    } while(0);


#define FATAL(...) \
    do { \
        fmt::print(stderr, "FATAL: {}\n", fmt::format(__VA_ARGS__)); \
        exit(1); \
    } while(0);


#else // ! HAVE_FMT

#define INFO(...)
#define WARNING(...)
#define DEBUG(...)
#define DEBUG2(...)
#define DEBUG3(...)
#define ERROR(...)
#define FATAL(...)

#endif // HAVE_FMT

#endif // __LOGGING_HPP__
