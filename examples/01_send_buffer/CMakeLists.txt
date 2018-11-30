find_package(Sanitizers)

add_executable(01_send_buffer_client client.cpp)
add_sanitizers(01_send_buffer_client)
target_link_libraries(01_send_buffer_client hermes)

add_executable(01_send_buffer_server server.cpp)
add_sanitizers(01_send_buffer_server)
target_link_libraries(01_send_buffer_server hermes)

add_custom_command(
  TARGET 01_send_buffer_client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_SOURCE_DIR}/examples/01_send_buffer/lipsum.txt
          ${CMAKE_CURRENT_BINARY_DIR}/lipsum.txt)
