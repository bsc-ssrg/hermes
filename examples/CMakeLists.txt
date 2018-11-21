find_package(Sanitizers)

add_executable(test_client test_client.cpp)
add_sanitizers(test_client)
target_link_libraries(test_client hermes)

add_executable(test_server test_server.cpp)
#uadd_sanitizers(test_server)
target_link_libraries(test_server hermes)
