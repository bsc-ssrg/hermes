# compilation flags
include(CheckCXXCompilerFlag)
macro(hermes_append_flag output testname flag ${ARGN})

    # As -Wno-* flags do not lead to build failure when there are no other
    # diagnostics, we check positive option to determine their applicability.
    # Of course, we set the original flag that is requested in the parameters.
    string(REGEX REPLACE "^-Wno-" "-W" alt ${flag})
    check_cxx_compiler_flag("${flag} ${ARGN}" ${testname})

    if (${testname})
        list(APPEND ${output} ${flag} ${ARGN})
    endif()
endmacro()

hermes_append_flag(HERMES_DEBUG_OPTIONS HERMES_HAS_WALL -Wall)
hermes_append_flag(HERMES_DEBUG_OPTIONS HERMES_HAS_WEXTRA -Wextra)
hermes_append_flag(HERMES_DEBUG_OPTIONS HERMES_HAS_O0 -O0)
hermes_append_flag(HERMES_DEBUG_OPTIONS HERMES_HAS_NO_INLINE -fno-inline)
hermes_append_flag(HERMES_DEBUG_OPTIONS HERMES_HAS_STACK_PROTECTOR_ALL -fstack-protector-all)
hermes_append_flag(HERMES_DEBUG_OPTIONS HERMES_HAS_G3 -g3)
hermes_append_flag(HERMES_DEBUG_OPTIONS HERMES_HAS_GGDB -ggdb)

if(HERMES_BUILD_WERROR)
    hermes_append_flag(WERROR_FLAG HERMES_HAS_WERROR -Werror)
    target_compile_options(hermes INTERFACE ${WERROR_FLAG})
endif()

if(HERMES_BUILD_PEDANTIC)
    hermes_append_flag(HERMES_PEDANTIC_COMPILE_FLAGS HERMES_HAS_WSHADOW -Wshadow) 
    hermes_append_flag(HERMES_PEDANTIC_COMPILE_FLAGS HERMES_HAS_PEDANTIC -pedantic) 
endif()
