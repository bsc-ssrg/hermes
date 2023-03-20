add_executable(02_recv_buffer_client "")
target_sources(02_recv_buffer_client
    PRIVATE
        client.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(02_recv_buffer_client
    PUBLIC
        hermes::hermes
    PRIVATE
        common
)
target_compile_features(02_recv_buffer_client PRIVATE cxx_std_14)

add_executable(02_recv_buffer_server "")
target_sources(02_recv_buffer_server
    PRIVATE
        server.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(02_recv_buffer_server
    PUBLIC
        hermes::hermes
    PRIVATE
        common
)
target_compile_features(02_recv_buffer_server PRIVATE cxx_std_14)

add_custom_command(
  TARGET 02_recv_buffer_client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/examples/02_recv_buffer/lipsum.txt
          ${CMAKE_CURRENT_BINARY_DIR}/lipsum.txt)
