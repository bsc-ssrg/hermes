find_package(Sanitizers)

add_executable(00_hello_world_client "")
add_sanitizers(00_hello_world_client)
target_sources(00_hello_world_client 
    PRIVATE
        client.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(00_hello_world_client hermes)

add_executable(00_hello_world_server "")
add_sanitizers(00_hello_world_server)
target_sources(00_hello_world_server 
    PRIVATE
        server.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(00_hello_world_server hermes)
