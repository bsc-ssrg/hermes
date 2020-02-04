#ifndef EXAMPLES_COMMON_LOGGING_FUNCTIONS_HPP
#define EXAMPLES_COMMON_LOGGING_FUNCTIONS_HPP

#ifdef HERMES_ENABLE_LOGGING

#include <hermes.hpp>
#include <fmt/format.h>
#include <string>

namespace common {

void
log_info(const std::string&, hermes::log::level, int, const std::string&, const
         std::string&, int);

void
log_warning(const std::string&, hermes::log::level, int, const std::string&, 
            const std::string&, int);

void
log_error(const std::string&, hermes::log::level, int, const std::string&, 
          const std::string&, int);

void
log_fatal(const std::string&, hermes::log::level, int, const std::string&, 
          const std::string&, int);

#ifdef HERMES_DEBUG_BUILD
void
log_debug(const std::string&, hermes::log::level, int, const std::string&, 
          const std::string&, int);
#endif

void
log_mercury(const std::string&, hermes::log::level, int, const std::string&, 
            const std::string&, int);

} // namespace common

#endif // HERMES_ENABLE_LOGGING
#endif // EXAMPLES_COMMON_LOGGING_FUNCTIONS_HPP
