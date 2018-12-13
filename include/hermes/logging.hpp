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
        if(logging::debug_output_level >= l) {                                \
            fmt::print(stdout, "[{}:{}()] [{}] {} {}\n", __FILE__,  __func__, \
                       std::this_thread::get_id(), "DEBUG:",                  \
                       fmt::format(__VA_ARGS__));                             \
        }                                                                     \
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

    
#define HERMES_ERROR(...) \
    do { \
        fmt::print(stderr, "ERROR: {}\n", fmt::format(__VA_ARGS__)); \
    } while(0);


#define HERMES_FATAL(...) \
    do { \
        fmt::print(stderr, "FATAL: {}\n", fmt::format(__VA_ARGS__)); \
        exit(1); \
    } while(0);


#else // ! HAVE_FMT

#ifndef HERMES_INFO
#define HERMES_INFO(...)
#endif // HERMES_INFO

#ifndef HERMES_WARNING
#define HERMES_WARNING(...)
#endif // HERMES_WARNING

#ifndef HERMES_DEBUG
#define HERMES_DEBUG(...)
#endif // HERMES_DEBUG

#ifndef HERMES_DEBUG2
#define HERMES_DEBUG2(...)
#endif // HERMES_DEBUG2

#ifndef HERMES_DEBUG3
#define HERMES_DEBUG3(...)
#endif // HERMES_DEBUG3

#ifndef HERMES_DEBUG4
#define HERMES_DEBUG4(...)
#endif // HERMES_DEBUG4

#ifndef HERMES_ERROR
#define HERMES_ERROR(...)
#endif // HERMES_ERROR

#ifndef HERMES_FATAL
#define HERMES_FATAL(...)
#endif // HERMES_FATAL

#endif // HAVE_FMT

#endif // __LOGGING_HPP__
