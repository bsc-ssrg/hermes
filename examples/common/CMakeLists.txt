add_library(common
    STATIC
    common.hpp
    logging_functions.cpp
    logging_functions.hpp
)

target_include_directories(common
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

target_compile_features(common PRIVATE cxx_std_14)
target_link_libraries(common PRIVATE hermes)
