#[=======================================================================[.rst:
FindMercury
---------

Find Mercury include dirs and libraries.

Use this module by invoking find_package with the form::

  find_package(Mercury
    [version] [EXACT]     # Minimum or EXACT version e.g. 0.6.2
    [REQUIRED]            # Fail with error if Mercury is not found
    )

Some variables one may set to control this module are::

  Mercury_DEBUG            - Set to ON to enable debug output from FindMercury.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Mercury::Mercury``
  The Mercury library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Mercury_FOUND``
  True if the system has the Mercury library.
``Mercury_VERSION``
  The version of the Mercury library which was found.
``Mercury_INCLUDE_DIRS``
  Include directories needed to use Mercury.
``Mercury_LIBRARIES``
  Libraries needed to link to Mercury.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``MERCURY_INCLUDE_DIR``
  The directory containing ``mercury.h``.
``MERCURY_LIBRARY``
  The path to the Mercury library.

#]=======================================================================]

#
# Print debug text if Mercury_DEBUG is set.
# Call example:
# _mercury_DEBUG_PRINT("debug message")
#
function(_mercury_DEBUG_PRINT text)
  if(Mercury_DEBUG)
    message(
      STATUS
        "[ ${CMAKE_CURRENT_LIST_FILE}:${CMAKE_CURRENT_FUNCTION_LIST_LINE} ] ${text}"
    )
  endif()
endfunction()

macro(_mercury_find_component _component_name)

  _mercury_debug_print("Searching for Mercury component: ${_component}")

  string(TOUPPER ${_component} _upper_component)

  # find component header
  find_path(
    ${_upper_component}_INCLUDE_DIR ${_component}
    NAMES ${_component}.h
    PATH_SUFFIXES include
    PATHS ${PC_MERCURY_INCLUDE_DIRS}
  )

  # find component library (release version)
  find_library(
    ${_upper_component}_LIBRARY_RELEASE
    NAMES ${_component}
    PATHS ${PC_MERCURY_LIBRARY_DIRS}
  )

  # find component library (debug version)
  find_library(
    ${_upper_component}_LIBRARY_DEBUG
    NAMES ${_component}_debug
    PATHS ${PC_MERCURY_LIBRARY_DIRS}
  )

  # initialize ${_upper_component}_LIBRARY (e.g. NA_LIBRARY)
  # with the appropriate library for this build configuration
  select_library_configurations(${_upper_component})

  _mercury_debug_print(
    "${_upper_component}_INCLUDE_DIR: ${${_upper_component}_INCLUDE_DIR}"
  )
  _mercury_debug_print(
    "${_upper_component}_LIBRARY: ${${_upper_component}_LIBRARY}"
  )

  # define an imported target for the component
  if (NOT TARGET Mercury::${_component})
    add_library(Mercury::${_component} UNKNOWN IMPORTED)
  endif ()

  if(${_upper_component}_LIBRARY_RELEASE)
    set_property(TARGET Mercury::${_component} APPEND PROPERTY
      IMPORTED_CONFIGURATIONS RELEASE)
    set_target_properties(Mercury::${_component} PROPERTIES
      IMPORTED_LOCATION_RELEASE "${${_upper_component}_LIBRARY_RELEASE}"
    )
  endif()

  if(${_upper_component}_LIBRARY_DEBUG)
    set_property(TARGET Mercury::${_component} APPEND PROPERTY
      IMPORTED_CONFIGURATIONS DEBUG)
    set_target_properties(Mercury::${_component} PROPERTIES
      IMPORTED_LOCATION_DEBUG "${${_upper_component}_LIBRARY_DEBUG}"
      IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
    )
  endif()

  set_target_properties(
    Mercury::${_component}
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MERCURY_INCLUDE_DIR}"
  )

  get_target_property(_configs Mercury::${_component} IMPORTED_CONFIGURATIONS)
  foreach(_config ${_configs})
    _mercury_DEBUG_PRINT("---> ${_config}")

  endforeach()

  set_property(TARGET Mercury::${_component} APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES )


  mark_as_advanced(${_upper_component}_INCLUDE_DIR ${_upper_component}_LIBRARY)

endmacro()

###############################################################################
# Find Mercury headers and library
###############################################################################

include(SelectLibraryConfigurations)

set(_mercury_components na mchecksum mercury_util mercury_hl)

# prevent repeating work if the main CMakeLists.txt already called
# find_package(PkgConfig)
if(NOT PKG_CONFIG_FOUND)
  find_package(PkgConfig REQUIRED)
endif()

if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_MERCURY QUIET mercury)

  # find mercury header
  find_path(
    MERCURY_INCLUDE_DIR
    NAMES mercury.h
    PATHS ${PC_MERCURY_INCLUDE_DIRS}
    PATH_SUFFIXES include
  )
  # find mercury library (release version)
  find_library(
    MERCURY_LIBRARY_RELEASE
    NAMES mercury
    PATHS ${PC_MERCURY_LIBRARY_DIRS}
  )

  # find mercury library (debug version)
  find_library(
    MERCURY_LIBRARY_DEBUG
    NAMES mercury_debug
    PATHS ${PC_MERCURY_LIBRARY_DIRS}
  )

  # initialize MERCURY_LIBRARY with the appropriate library for this build
  # configuration
  select_library_configurations(MERCURY)

  # Mercury_VERSION needs to be set to the appropriate value for the call
  # to find_package_handle_standard_args() below to work as expected
  set(Mercury_VERSION ${PC_MERCURY_VERSION})
endif()

mark_as_advanced(MERCURY_INCLUDE_DIR MERCURY_LIBRARY)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Mercury
  FOUND_VAR Mercury_FOUND
  REQUIRED_VARS MERCURY_LIBRARY MERCURY_INCLUDE_DIR
  VERSION_VAR Mercury_VERSION
)

if(Mercury_FOUND)
  set(Mercury_INCLUDE_DIRS ${MERCURY_INCLUDE_DIR})
  set(Mercury_LIBRARIES ${MERCURY_LIBRARY})

  if(NOT TARGET Mercury::Mercury)
    add_library(Mercury::Mercury UNKNOWN IMPORTED)
  endif()

  if(MERCURY_LIBRARY_RELEASE)
    set_property(
      TARGET Mercury::Mercury
      APPEND
      PROPERTY IMPORTED_CONFIGURATIONS RELEASE
    )
    set_target_properties(
      Mercury::Mercury PROPERTIES IMPORTED_LOCATION_RELEASE
                                  "${MERCURY_LIBRARY_RELEASE}"
    )
  endif()

  if(MERCURY_LIBRARY_DEBUG)
    set_property(
      TARGET Mercury::Mercury
      APPEND
      PROPERTY IMPORTED_CONFIGURATIONS DEBUG
    )
    set_target_properties(
      Mercury::Mercury PROPERTIES IMPORTED_LOCATION_DEBUG
                                  "${MERCURY_LIBRARY_DEBUG}"
    )
  endif()

  set_target_properties(
    Mercury::Mercury
    PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_MERCURY_CFLAGS_OTHER}"
               INTERFACE_INCLUDE_DIRECTORIES "${MERCURY_INCLUDE_DIR}"

  )

  ###############################################################################
  # Find headers and libraries for additional Mercury components
  ###############################################################################
  foreach(_component ${_mercury_components})
    _mercury_find_component(${_component})

    set_target_properties(Mercury::Mercury PROPERTIES
      INTERFACE_LINK_LIBRARIES Mercury::${_component})
  endforeach()
endif()
