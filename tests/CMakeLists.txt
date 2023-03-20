###############################################################################
# Tests for examples/00_hello_world
###############################################################################
add_test(
    NAME start_hello_world_server
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/runner.sh
            start
            hello_world.pid
            "$<TARGET_FILE:00_hello_world_server>"
            ${HERMES_TRANSPORT_PROTOCOL}://${HERMES_BIND_ADDRESS}:${HERMES_BIND_PORT})

set_tests_properties(start_hello_world_server
    PROPERTIES FIXTURES_SETUP hello_world_server)

add_test(
    NAME stop_hello_world_server
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/runner.sh
            stop
            TERM
            hello_world.pid)

set_tests_properties(stop_hello_world_server
    PROPERTIES FIXTURES_CLEANUP hello_world_server)

add_test(
    NAME hello_world_client
    COMMAND 00_hello_world_client
            ${HERMES_TRANSPORT_PROTOCOL}://${HERMES_BIND_ADDRESS}:${HERMES_BIND_PORT})

set_tests_properties(hello_world_client
    PROPERTIES FIXTURES_REQUIRED hello_world_server)


###############################################################################
# Tests for examples/01_send_buffer
###############################################################################
add_test(
        NAME start_send_buffer_server
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/runner.sh
        start
        send_buffer.pid
        "$<TARGET_FILE:01_send_buffer_server>"
        ${HERMES_TRANSPORT_PROTOCOL}://${HERMES_BIND_ADDRESS}:${HERMES_BIND_PORT})

set_tests_properties(start_send_buffer_server
        PROPERTIES FIXTURES_SETUP send_buffer_server)

add_test(
        NAME stop_send_buffer_server
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/runner.sh stop TERM send_buffer.pid)

set_tests_properties(stop_send_buffer_server
        PROPERTIES FIXTURES_CLEANUP send_buffer_server)

add_test(
        NAME send_buffer_client
        COMMAND 01_send_buffer_client
            ${HERMES_TRANSPORT_PROTOCOL}://${HERMES_BIND_ADDRESS}:${HERMES_BIND_PORT})

set_tests_properties(send_buffer_client
        PROPERTIES FIXTURES_REQUIRED send_buffer_server)


###############################################################################
# Tests for examples/02_recv_buffer
###############################################################################
add_test(
        NAME start_recv_buffer_server
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/runner.sh
        start
        recv_buffer.pid
        "$<TARGET_FILE:02_recv_buffer_server>"
        ${HERMES_TRANSPORT_PROTOCOL}://${HERMES_BIND_ADDRESS}:${HERMES_BIND_PORT})

set_tests_properties(start_recv_buffer_server
        PROPERTIES FIXTURES_SETUP recv_buffer_server)

add_test(
        NAME stop_recv_buffer_server
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/runner.sh stop TERM recv_buffer.pid)

set_tests_properties(stop_recv_buffer_server
        PROPERTIES FIXTURES_CLEANUP recv_buffer_server)

add_test(
        NAME recv_buffer_client
        COMMAND 02_recv_buffer_client
        ${HERMES_TRANSPORT_PROTOCOL}://${HERMES_BIND_ADDRESS}:${HERMES_BIND_PORT})

set_tests_properties(recv_buffer_client
        PROPERTIES FIXTURES_REQUIRED recv_buffer_server)
