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
  message(STATUS "CMake version: ${CMAKE_VERSION}")
endif ()


if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

### # Joins arguments and places the results in ${result_var}.
### function(join result_var)
###   set(result )
###   foreach (arg ${ARGN})
###     set(result "${result}${arg}")
###   endforeach ()
###   set(${result_var} "${result}" PARENT_SCOPE)
### endfunction()
### 
### # Set the default CMAKE_BUILD_TYPE to Release.
### # This should be done before the project command since the latter can set
### # CMAKE_BUILD_TYPE itself (it does so for nmake).
### if (NOT CMAKE_BUILD_TYPE)
###   join(doc "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or "
###            "CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
###   set(CMAKE_BUILD_TYPE Release CACHE STRING ${doc})
### endif ()

# Options that control how to build
option(HERMES_PEDANTIC "Enable extra warnings and expensive tests." OFF)
option(HERMES_WERROR "Halt the compilation with an error on compiler warnings." OFF)
option(HERMES_EXAMPLES "Build the examples." OFF)
option(HERMES_LOGGING "Enable logging messages (using the fmt library)" OFF)

# Options that control generation of various targets.
option(HERMES_DOC "Generate the doc target." ${MASTER_PROJECT})
option(HERMES_INSTALL "Generate the install target." ${MASTER_PROJECT})
option(HERMES_TEST "Generate the test target." ${MASTER_PROJECT})

project(HERMES)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/externals/sanitizers-cmake/cmake")

# Export compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(hermes_env)
include(hermes_flags)

add_library(hermes INTERFACE)

if (HERMES_WERROR)
  target_compile_options(hermes PRIVATE ${WERROR_FLAG})
endif ()

if (HERMES_PEDANTIC)
  target_compile_options(hermes INTERFACE ${PEDANTIC_COMPILE_FLAGS})
endif ()


target_include_directories(hermes INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
target_include_directories(hermes SYSTEM INTERFACE
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)

if(HERMES_EXAMPLES)
    add_subdirectory(examples)
endif()

# 
find_package(Mercury REQUIRED)

if (HERMES_LOGGING)
  find_package(fmt 5.0)
  
  if(fmt_FOUND)
    target_compile_definitions(hermes INTERFACE HAVE_FMT=1 HERMES_DEBUG_BUILD=1)
    target_link_libraries(hermes INTERFACE fmt)
  else()
    target_compile_definitions(hermes INTERFACE HAVE_FMT=0)
  endif()
endif()

set(RPC_PROTOCOL "ofi+tcp" CACHE STRING "Communication plugin used for RPCs")
set_property(CACHE RPC_PROTOCOL PROPERTY STRINGS
   "bmi+tcp"
   "ofi+tcp"
   "ofi+verbs"
   "ofi+psm2"
   "cci+verbs"
)
message(STATUS "RPC protocol: '${RPC_PROTOCOL}'")

target_link_libraries(hermes INTERFACE mercury)

find_package(Sanitizers)
