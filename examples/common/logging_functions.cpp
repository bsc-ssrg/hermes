#include "logging_functions.hpp"

#ifdef HERMES_ENABLE_LOGGING

namespace common {

void
log_info(const std::string& msg, hermes::log::level l, int severity, 
         const std::string& file, const std::string& func, int lineno) {

    (void) l;
    (void) severity;
    (void) file;
    (void) func;
    (void) lineno;

    fmt::print(stdout, "INFO: {}", msg);
}

void
log_warning(const std::string& msg, hermes::log::level l, int severity, 
            const std::string& file, const std::string& func, int lineno) {

    (void) l;
    (void) severity;
    (void) file;
    (void) func;
    (void) lineno;

    fmt::print(stdout, "WARNING: {}", msg);
}

void
log_error(const std::string& msg, hermes::log::level l, int severity, 
          const std::string& file, const std::string& func, int lineno) {

    (void) l;
    (void) severity;
    (void) file;
    (void) func;
    (void) lineno;

    fmt::print(stdout, "ERROR: {}", msg);
}

void
log_fatal(const std::string& msg, hermes::log::level l, int severity, 
          const std::string& file, const std::string& func, int lineno) {

    (void) l;
    (void) severity;
    (void) file;
    (void) func;
    (void) lineno;

    fmt::print(stderr, "FATAL: {}", msg);
}

#ifdef HERMES_DEBUG_BUILD
void
log_debug(const std::string& msg, hermes::log::level l, int severity, 
          const std::string& file, const std::string& func, int lineno) {

    (void) l;
    (void) severity;
    (void) lineno;

    if(severity > 2) {
        return;
    }

    fmt::print(stdout, "[{}:{}()] [{}] {}{}: {}\n", file,  func,
               std::this_thread::get_id(), "DEBUG", severity, msg);
}
#endif

void
log_mercury(const std::string& msg, hermes::log::level l, int severity, 
            const std::string& file, const std::string& func, int lineno) {

    (void) l;
    (void) severity;
    (void) file;
    (void) func;
    (void) lineno;

    fmt::print(stderr, "MERCURY: {}", msg);
}

} // namespace common

#endif // HERMES_ENABLE_LOGGING
