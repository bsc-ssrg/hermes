include(CMakeDependentOption)

# Options that control how to build
option(HERMES_BUILD_EXAMPLES "Build the examples." OFF)

# Options controlling optional features
option(HERMES_LOGGING "Enable logging messages (using the fmt library)" OFF)
cmake_dependent_option(HERMES_LOGGING_FMT_HEADER_ONLY "Use fmt library as header-only" OFF "HERMES_LOGGING" OFF)
option(HERMES_MARGO_COMPATIBLE_RPCS "Enable features for compatibility with Margo servers (not required if Margo < v0.5.2)" OFF)

# Options that control generation of various targets.
option(HERMES_DOC "Generate the doc target." ${MASTER_PROJECT})
option(HERMES_INSTALL "Generate the install target." ${MASTER_PROJECT})
option(HERMES_ENABLE_TESTS "Generate the test target." ${MASTER_PROJECT})

# Options controlling network connections
set(HERMES_TRANSPORT_PROTOCOL
    "ofi+tcp" CACHE STRING
    "Change the default transport protocol for the ${PROJECT_NAME} server (default: ofgi+tcp)")

set(HERMES_BIND_ADDRESS
    "127.0.0.1" CACHE STRING
    "Define the bind address for the ${PROJECT_NAME} server (default: 127.0.0.1)")

set(HERMES_BIND_PORT
    "52000" CACHE STRING
    "Define the bind port for the ${PROJECT_NAME} server (default: 52000)")
