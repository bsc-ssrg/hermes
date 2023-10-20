#[=======================================================================[.rst:
FindMargo-hg-shim
---------

Find Margo shim library include dirs and libraries.

Use this module by invoking find_package with the form::

  find_package(Margo-hg-shim
    [version] [EXACT]     # Minimum or EXACT version e.g. 0.6.2
    [REQUIRED]            # Fail with error if Margo is not found
    )

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Margo::Margo-hg-shim``
  The Margo-hg-shim library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Margo-hg-shim_FOUND``
  True if the system has the Margo-hg-shim library.
``Margo_hg_shim_VERSION``
  The version of the Margo-hg-shim library which was found.
``Margo_HG_SHIM_INCLUDE_DIRS``
  Include directories needed to use Margo-hg-shim.
``Margo_HG_SHIM_LIBRARIES``
  Libraries needed to link to Margo-hg-shim.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Margo_HG_SHIM_INCLUDE_DIR``
  The directory containing ``margo.h``.
``Margo_HG_SHIM_LIBRARY``
  The path to the Margo library.

#]=======================================================================]

function(_get_pkgconfig_paths target_var)
  set(_lib_dirs)
  if (NOT DEFINED CMAKE_SYSTEM_NAME
      OR (CMAKE_SYSTEM_NAME MATCHES "^(Linux|kFreeBSD|GNU)$"
      AND NOT CMAKE_CROSSCOMPILING)
      )
    if (EXISTS "/etc/debian_version") # is this a debian system ?
      if (CMAKE_LIBRARY_ARCHITECTURE)
        list(APPEND _lib_dirs "lib/${CMAKE_LIBRARY_ARCHITECTURE}/pkgconfig")
      endif ()
    else ()
      # not debian, check the FIND_LIBRARY_USE_LIB32_PATHS and FIND_LIBRARY_USE_LIB64_PATHS properties
      get_property(uselib32 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS)
      if (uselib32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        list(APPEND _lib_dirs "lib32/pkgconfig")
      endif ()
      get_property(uselib64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
      if (uselib64 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND _lib_dirs "lib64/pkgconfig")
      endif ()
      get_property(uselibx32 GLOBAL PROPERTY FIND_LIBRARY_USE_LIBX32_PATHS)
      if (uselibx32 AND CMAKE_INTERNAL_PLATFORM_ABI STREQUAL "ELF X32")
        list(APPEND _lib_dirs "libx32/pkgconfig")
      endif ()
    endif ()
  endif ()
  if (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND NOT CMAKE_CROSSCOMPILING)
    list(APPEND _lib_dirs "libdata/pkgconfig")
  endif ()
  list(APPEND _lib_dirs "lib/pkgconfig")
  list(APPEND _lib_dirs "share/pkgconfig")

  set(_extra_paths)
  list(APPEND _extra_paths ${CMAKE_PREFIX_PATH})
  list(APPEND _extra_paths ${CMAKE_FRAMEWORK_PATH})
  list(APPEND _extra_paths ${CMAKE_APPBUNDLE_PATH})

  # Check if directories exist and eventually append them to the
  # pkgconfig path list
  foreach (_prefix_dir ${_extra_paths})
    foreach (_lib_dir ${_lib_dirs})
      if (EXISTS "${_prefix_dir}/${_lib_dir}")
        list(APPEND _pkgconfig_paths "${_prefix_dir}/${_lib_dir}")
        list(REMOVE_DUPLICATES _pkgconfig_paths)
      endif ()
    endforeach ()
  endforeach ()

  set("${target_var}"
      ${_pkgconfig_paths}
      PARENT_SCOPE
      )
endfunction()

# prevent repeating work if the main CMakeLists.txt already called
# find_package(PkgConfig)
if (NOT PKG_CONFIG_FOUND)
  find_package(PkgConfig)
endif ()

if (PKG_CONFIG_FOUND)
  pkg_check_modules(PC_MARGO_HG_SHIM QUIET margo-hg-shim)

  find_path(
      MARGO_HG_SHIM_INCLUDE_DIR
      NAMES margo-hg-shim.h
      PATHS ${PC_MARGO_HG_SHIM_INCLUDE_DIRS}
      PATH_SUFFIXES include
  )

  find_library(
      MARGO_HG_SHIM_LIBRARY
      NAMES margo-hg-shim
      PATHS ${PC_MARGO_HG_SHIM_LIBRARY_DIRS}
  )

  set(Margo_hg_shim_VERSION ${PC_MARGO_HG_SHIM_VERSION})
else ()
  find_path(
      MARGO_HG_SHIMINCLUDE_DIR
      NAMES margo-hg-shim.h
      PATH_SUFFIXES include
  )

  find_library(MARGO_HG_SHIM_LIBRARY NAMES margo-hg-shim)

  # even if pkg-config is not available, but Margo still installs a .pc file
  # that we can use to retrieve library information from. Try to find it at all
  # possible pkgconfig subfolders (depending on the system).
  _get_pkgconfig_paths(_pkgconfig_paths)

  find_file(_margo_hg_shim_pc_file margo-hg-shim.pc PATHS "${_pkgconfig_paths}")

  if (NOT _margo_hg_shim_pc_file)
    message(
        FATAL_ERROR
        "ERROR: Could not find 'margo_hg_shim.pc' file. Unable to determine library version"
    )
  endif ()

  file(STRINGS "${_margo_hg_shim_pc_file}" _margo_hg_shim_pc_file_contents REGEX "Version: ")

  if ("${_margo_hg_shim_pc_file_contents}" MATCHES "Version: ([0-9]+\\.[0-9]+\\.[0-9])")
    set(Margo_hg_shim_VERSION ${CMAKE_MATCH_1})
  else ()
    message(FATAL_ERROR "ERROR: Failed to determine library version")
  endif ()

  unset(_pkg_config_paths)
  unset(_margo_hg_shim_pc_file_contents)
endif ()

mark_as_advanced(MARGO_HG_SHIM_INCLUDE_DIR MARGO_HG_SHIM_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Margo-hg-shim
    FOUND_VAR Margo-hg-shim_FOUND
    REQUIRED_VARS MARGO_HG_SHIM_LIBRARY MARGO_HG_SHIM_INCLUDE_DIR
    VERSION_VAR Margo_hg_shim_VERSION
)

if (Margo-hg-shim_FOUND)
  set(Margo_HG_SHIM_INCLUDE_DIRS ${MARGO_HG_SHIM_INCLUDE_DIR})
  set(Margo_HG_SHIM_LIBRARIES ${MARGO_HG_SHIM_LIBRARY})
  if (NOT TARGET Margo::Margo-hg-shim)
    add_library(Margo::Margo-hg-shim UNKNOWN IMPORTED)
    set_target_properties(
        Margo::Margo-hg-shim
        PROPERTIES IMPORTED_LOCATION "${MARGO_HG_SHIM_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MARGO_HG_SHIM_INCLUDE_DIR}"
    )
  endif ()
endif ()
