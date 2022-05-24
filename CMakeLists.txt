cmake_minimum_required(VERSION 3.6.0)

# Use newer policies if available, up to most recent tested version of CMake.
if(${CMAKE_VERSION} VERSION_LESS 3.11)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.11)
endif()

# Determine if hermes is built as a subproject (using add_subdirectory)
# or if it is the master project.
set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
  message(STATUS "[hermes] CMake version: ${CMAKE_VERSION}")
endif ()

# set default build type and also populate a list of available options
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "[hermes] Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# define project
project(HERMES 
        VERSION 0.4.5)

message(STATUS "[hermes] Version: ${HERMES_VERSION}")
message(STATUS "[hermes] Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/externals/sanitizers-cmake/cmake")

# Export compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(hermes INTERFACE)

# building the library requires (at least) C++ 11
target_compile_features(hermes INTERFACE cxx_std_11)

include(hermes_options)
include(hermes_flags)

# set definitions and flags for debug builds
target_compile_definitions(hermes INTERFACE 
    $<$<CONFIG:Debug>:HERMES_DEBUG_BUILD>)

target_compile_options(hermes INTERFACE 
    $<$<CONFIG:Debug>:${HERMES_DEBUG_OPTIONS}>)

# set flags common to all builds
if(HERMES_BUILD_WERROR)
    target_compile_options(hermes INTERFACE ${HERMES_WERROR_FLAG})
endif()

if(HERMES_BUILD_PEDANTIC)
    target_compile_options(hermes INTERFACE ${HERMES_PEDANTIC_COMPILE_FLAGS})
endif()

target_include_directories(hermes INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(hermes SYSTEM INTERFACE
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

if(HERMES_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 
find_package(Mercury REQUIRED)
target_link_libraries(hermes INTERFACE Mercury::Mercury)

if(HERMES_LOGGING)
  if(HERMES_LOGGING_FMT_HEADER_ONLY)
    set(HERMES_FMT_LINK_TYPE "(header only)")
  else()
    set(HERMES_FMT_LINK_TYPE "(library)")
  endif()

  target_compile_definitions(hermes INTERFACE HERMES_ENABLE_LOGGING)

  if(HERMES_LOGGING_USE_BUNDLED_FMT)
    message(STATUS "[hermes] Logging enabled using bundled fmt ${HERMES_FMT_LINK_TYPE}")
    add_subdirectory(externals/fmt)
    target_compile_definitions(hermes INTERFACE HERMES_LOGGING_USE_BUNDLED_FMT)
  else()
    if(TARGET fmt::fmt AND TARGET fmt::fmt-header-only)
      message(STATUS "[hermes] Logging enabled using external fmt ${HERMES_FMT_LINK_TYPE}")
    else()
      message(STATUS "[hermes] Logging enabled using installed fmt ${HERMES_FMT_LINK_TYPE}")
      find_package(fmt 5.3 REQUIRED)
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

find_package(Sanitizers)
