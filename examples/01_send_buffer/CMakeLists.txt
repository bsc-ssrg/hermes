find_package(Sanitizers)

add_executable(01_send_buffer_client "")
target_sources(01_send_buffer_client 
    PRIVATE
        client.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(01_send_buffer_client
    PUBLIC
        hermes
    PRIVATE
        common
)
add_sanitizers(01_send_buffer_client)
target_compile_features(01_send_buffer_client PRIVATE cxx_std_14)

add_executable(01_send_buffer_server "")
target_sources(01_send_buffer_server 
    PRIVATE
        server.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(01_send_buffer_server 
    PUBLIC
        hermes
    PRIVATE
        common
)
add_sanitizers(01_send_buffer_server)
target_compile_features(01_send_buffer_server PRIVATE cxx_std_14)

add_custom_command(
  TARGET 01_send_buffer_client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/examples/01_send_buffer/lipsum.txt
          ${CMAKE_CURRENT_BINARY_DIR}/lipsum.txt)
