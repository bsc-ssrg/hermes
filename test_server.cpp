#include <unistd.h>

#include "rpcs.hpp"
#include "hermes.hpp"
#include "logging.hpp"

hermes::send_file_retval
send_file_handler(const hermes::request& req, 
                  const hermes::send_file_args& args) {

    INFO("RPC [send_file] received");

    // hermes::send_file_out_t out;
    // out.ret_val = 42;

    INFO("Callback invoked: {}(\"{}\") = {}!", 
         __FUNCTION__, args.get_pathname(), "?");

    return {};
}

struct example_class {

    const int m_retval = 36;

    hermes::send_message_retval
    send_message_handler(const hermes::request& req, 
                         const hermes::send_message_args& args) {

        INFO("RPC [send_message] received");

        //hermes::send_message_out_t out;
        //out.ret_val = m_retval;

        INFO("Member callback invoked: {}(\"{}\") = {}!", 
             __FUNCTION__, args.get_message(), "?");

        return {};
    }
};

int
main(int argc, char* argv[]) {

    using hermes::async_engine;
    using hermes::event;

    (void) argc;
    (void) argv;

    try {
        hermes::async_engine hg(hermes::transport_type::bmi_tcp, true);

        const auto send_buffer_handler = 
            [&](const hermes::request& req,
                const hermes::send_buffer_args& args) {

                // hermes::send_buffer_retval out;
                // out.ret_val = 54;

                INFO("RPC [send_buffer] received");

                INFO("Lambda callback invoked: {}(\"{}\") = {}!", 
                     __FUNCTION__, args.get_pathname(), "?");

                auto remote_buffers = args.get_exposed_memory();

                // 1. allocate local buffers according to info in remote_buffers
                // 2. local_buffers = expose(bufs)
                // 3. hermes::transfer(remote_buffers, local_buffers);

                std::vector<hermes::mutable_buffer> bufvec;
                bufvec.reserve(remote_buffers.count());

                for(auto&& remote_buf : args.get_exposed_memory()) {
                    std::size_t remote_size = remote_buf.size();
                    char* data = new char[remote_size];
                    bufvec.emplace_back(data, remote_size);
                }

                auto local_buffers = 
                    hg.expose(bufvec, hermes::access_mode::write_only);

                INFO("Pulling remote data");

                // this lambda will be invoked when the pull transfer completes
                // (we capture bufvec by reference to verify that remote data
                // was actually copied into the buffers, but it's not necessary
                // in real code)
                const auto do_pull_completion = 
                    [&bufvec](const hermes::request& req) {

                    INFO("Pull successful!");

                    for(auto&& local_buf : bufvec) {
                        INFO("Buffer size: {}", local_buf.size());
                        INFO("Buffer contents: {}", 
                                reinterpret_cast<char*>(local_buf.data()));
                    }

                    // if(req.requires_response()) {
                    //     hg.respond(req, ...);
                    // }
                };

                hg.pull(req, 
                        remote_buffers, 
                        local_buffers, 
                        do_pull_completion);

                return hermes::send_buffer_retval{};
            };

        hg.register_handler<hermes::rpc::send_buffer>(
                event::on_arrival,
                send_buffer_handler);

        hg.register_handler<hermes::rpc::send_file>(
                event::on_arrival,
                send_file_handler);

        example_class ex;

        hg.register_handler<hermes::rpc::send_message>(
                event::on_arrival,
                std::bind(&example_class::send_message_handler, 
                          ex, std::placeholders::_1, std::placeholders::_2));

        hg.run();

        while(true) {
            sleep(1);
        }
    } 
    catch(const std::exception& ex) {
        FATAL("{}", ex.what());
    }

    return 0;
}
