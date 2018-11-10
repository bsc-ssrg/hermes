#include <unistd.h>

#include "rpcs.hpp"
#include "hermes.hpp"
#include "logging.hpp"

void
send_file_handler(hermes::request&& req, 
                  const hermes::send_file_args& args) {

    (void) req;
    (void) args;

    INFO("RPC [send_file] received");

    // hermes::send_file_out_t out;
    // out.ret_val = 42;

    INFO("Callback invoked: {}(\"{}\") = {}!", 
         __FUNCTION__, args.get_pathname(), "?");
}

struct example_class {

    example_class(const hermes::async_engine& hg) :
        m_hg(hg) { }

    const int m_retval = 36;

    void
    send_message_handler(hermes::request&& req, 
                         const hermes::send_message_args& args) {

        (void) req;

        INFO("RPC [send_message] received");

        //hermes::send_message_out_t out;
        //out.ret_val = m_retval;

        INFO("Member callback invoked: {}(\"{}\") = {}!", 
             __FUNCTION__, args.get_message(), "?");

        if(req.requires_response()) {
            m_hg.respond<hermes::rpc::send_message>(std::move(req), 42);

            // other valid options:
            //
            // hermes::send_message_retval rv(42);
            // m_hg.respond<hermes::rpc::send_message>(std::move(req), rv);
            //
            // m_hg.respond<hermes::rpc::send_message>(
            //                  std::move(req), 
            //                  hermes::send_message_retval{42});
        }
    }

    const hermes::async_engine& m_hg;
};

int
main(int argc, char* argv[]) {

    using hermes::async_engine;

    (void) argc;
    (void) argv;

    try {
        hermes::async_engine hg(hermes::transport::ofi_tcp, true);

        const auto send_buffer_handler = 
            [&](hermes::request&& req,
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

                // TODO: remove the need to expose local buffers and just pass
                // the bufvec directly to async_pull
                auto local_buffers = 
                    hg.expose(bufvec, hermes::access_mode::write_only);

                INFO("Pulling remote data");

                // this lambda will be invoked when the pull transfer completes
                // (we capture bufvec by reference to verify that remote data
                // was actually copied into the buffers, but it's not necessary
                // in real code)
                const auto do_pull_completion = 
                    [&bufvec, &hg](hermes::request&& req) {

                    INFO("Pull successful!");

                    for(auto&& local_buf : bufvec) {
                        INFO("Buffer size: {}", local_buf.size());
                        INFO("Buffer contents: {}", 
                                reinterpret_cast<char*>(local_buf.data()));
                    }

                    if(req.requires_response()) {
                        hermes::send_buffer_retval rv(42);
                        hg.respond<hermes::rpc::send_buffer>(std::move(req), rv);
                    }
                };

                hg.async_pull(std::move(req), 
                              local_buffers, 
                              do_pull_completion);
            };

        hg.register_handler<hermes::rpc::send_buffer>(send_buffer_handler);

        hg.register_handler<hermes::rpc::send_file>(send_file_handler);

        example_class ex(hg);

        hg.register_handler<hermes::rpc::send_message>(
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
