find_package(Sanitizers)

add_executable(00_hello_world_client "")
target_sources(00_hello_world_client 
    PRIVATE
        client.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(00_hello_world_client 
    PUBLIC
        hermes
    PRIVATE
        common
)
add_sanitizers(00_hello_world_client)
target_compile_features(00_hello_world_client PRIVATE cxx_std_14)

add_executable(00_hello_world_server "")
target_sources(00_hello_world_server 
    PRIVATE
        server.cpp
        rpcs.hpp
        rpcs.cpp
)
target_link_libraries(00_hello_world_server 
    PUBLIC
        hermes
    PRIVATE
        common
)
add_sanitizers(00_hello_world_server)
target_compile_features(00_hello_world_server PRIVATE cxx_std_14)
