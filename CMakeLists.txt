# 3.13+ so that we have a sensible handling of relative paths
cmake_minimum_required(VERSION 3.13.0)

# Define the project
project(hermes
        VERSION 0.4.5
        LANGUAGES C CXX)

# Print basic information about the project
message(STATUS "[${PROJECT_NAME}] Version: ${PROJECT_VERSION}")
message(STATUS "[${PROJECT_NAME}] Build type: ${CMAKE_BUILD_TYPE}")

if(HERMES_TEST)
  # Tests need examples to be built
  set(HERMES_BUILD_EXAMPLES ON)
  enable_testing()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Let CMake know where to find or custom .cmake scripts
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Export compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the library
add_library(hermes INTERFACE)
add_library(hermes::hermes ALIAS hermes)

# building the library requires (at least) C++ 11
target_compile_features(hermes INTERFACE cxx_std_11)

include(hermes_options)
include(hermes_flags)

# set definitions and flags for debug builds
target_compile_definitions(hermes INTERFACE
    $<$<CONFIG:Debug>:HERMES_DEBUG_BUILD>)

target_compile_options(hermes INTERFACE
    $<$<CONFIG:Debug>:${HERMES_DEBUG_OPTIONS}>)

target_include_directories(hermes INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(hermes SYSTEM INTERFACE
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

# Find dependencies
find_package(Threads REQUIRED)
find_package(Mercury REQUIRED)
target_link_libraries(hermes INTERFACE Threads::Threads Mercury::Mercury)

if(HERMES_LOGGING)
  if(HERMES_LOGGING_FMT_HEADER_ONLY)
    set(HERMES_FMT_LINK_TYPE "(header only)")
  else()
    set(HERMES_FMT_LINK_TYPE "(library)")
  endif()

  target_compile_definitions(hermes INTERFACE HERMES_ENABLE_LOGGING)

  if(TARGET fmt::fmt AND TARGET fmt::fmt-header-only)
    message(STATUS "[${PROJECT_NAME}] Logging enabled using external fmt ${HERMES_FMT_LINK_TYPE}")
  else()
    find_package(fmt 5.3)

    if(fmt_FOUND)
      message(STATUS "[${PROJECT_NAME}] Logging enabled using installed fmt ${HERMES_FMT_LINK_TYPE}")
    else()
      message(STATUS "[${PROJECT_NAME}] Downloading and building {fmt}")
      FetchContent_Declare(
              fmt
              GIT_REPOSITORY https://github.com/fmtlib/fmt
              GIT_TAG d141cdbeb0fb422a3fb7173b285fd38e0d1772dc # v8.0.1
              GIT_SHALLOW ON
              GIT_PROGRESS ON
      )
      FetchContent_MakeAvailable(fmt)
    endif()
  endif()

  target_link_libraries(hermes
      INTERFACE
      $<IF:$<BOOL:${HERMES_LOGGING_FMT_HEADER_ONLY}>,
          fmt::fmt-header-only,
          fmt::fmt>
  )

  unset(HERMES_FMT_LINK_TYPE)
endif()

if(HERMES_MARGO_COMPATIBLE_RPCS)
  target_compile_definitions(hermes INTERFACE HERMES_MARGO_COMPATIBLE_MODE)
endif()

if(HERMES_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(HERMES_ENABLE_TESTS)
  add_subdirectory(tests)
endif()

