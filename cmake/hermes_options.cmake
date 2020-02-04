include(CMakeDependentOption)

# Options that control how to build
option(HERMES_BUILD_PEDANTIC "Enable extra warnings and expensive tests." OFF)
option(HERMES_BUILD_WERROR "Halt the compilation with an error on compiler warnings." OFF)
option(HERMES_BUILD_EXAMPLES "Build the examples." OFF)

# Options controlling optional features
option(HERMES_LOGGING "Enable logging messages (using the fmt library)" OFF)
cmake_dependent_option(HERMES_LOGGING_FMT_HEADER_ONLY "Use fmt library as header-only" OFF "HERMES_LOGGING" OFF)
cmake_dependent_option(HERMES_LOGGING_FMT_USE_BUNDLED "Use the fmt library bundled with hermes instead of the one installed on the system" ON "HERMES_LOGGING" OFF)
option(HERMES_MARGO_COMPATIBLE_RPCS "Enable features for compatibility with Margo servers (not required if Margo < v0.5.2)" OFF)

# Options that control generation of various targets.
option(HERMES_DOC "Generate the doc target." ${MASTER_PROJECT})
option(HERMES_INSTALL "Generate the install target." ${MASTER_PROJECT})
option(HERMES_TEST "Generate the test target." ${MASTER_PROJECT})

