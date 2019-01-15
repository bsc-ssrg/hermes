find_package(Sanitizers)

add_executable(01_send_buffer_client "")
add_sanitizers(01_send_buffer_client)
target_sources(01_send_buffer_client 
    PRIVATE
        client.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(01_send_buffer_client hermes)

add_executable(01_send_buffer_server "")
add_sanitizers(01_send_buffer_server)
target_sources(01_send_buffer_server 
    PRIVATE
        server.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(01_send_buffer_server hermes)

add_custom_command(
  TARGET 01_send_buffer_client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/examples/01_send_buffer/lipsum.txt
          ${CMAKE_CURRENT_BINARY_DIR}/lipsum.txt)
