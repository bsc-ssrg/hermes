#include <unistd.h>

#include "rpcs.hpp"
#include <hermes.hpp>

using hermes::request;
using hermes::async_engine;
using hermes::rpc;
using hermes::send_file_args;
using hermes::send_message_args;
using hermes::send_buffer_args;

void
send_file_handler(request<send_file_args>&& req) {

    (void) req;

    send_file_args args = req.args();

    INFO("RPC [send_file] received");

    // hermes::send_file_out_t out;
    // out.ret_val = 42;

    INFO("Callback invoked: {}(\"{}\") = {}!", 
         __FUNCTION__, args.pathname(), "?");
}

struct example_class {

    example_class(const async_engine& hg) :
        m_hg(hg) { }

    const int m_retval = 36;

    void
    send_message_handler(request<send_message_args>&& req) {

        send_message_args args = req.args();

        INFO("RPC received:");
        INFO("    type: send_message,"); 
        INFO("    args: \"{}\"", args.message());

        if(req.requires_response()) {
            /*******************************************************************
             * Other valid options:
             *
             * hermes::send_message_retval rv(42);
             * m_hg.respond<hermes::rpc::send_message>(std::move(req), rv);
             *
             * m_hg.respond<hermes::rpc::send_message>(
             *                  std::move(req), 
             *                  hermes::send_message_retval{42});
             ******************************************************************/
            m_hg.respond<rpc::send_message>(std::move(req), 0);
            INFO("  Response sent with value {}", 0);
        }
    }

    const async_engine& m_hg;
};

int
main(int argc, char* argv[]) {

    using hermes::async_engine;

    (void) argc;
    (void) argv;

    try {
        hermes::async_engine hg(hermes::transport::ofi_tcp, true);

        const auto send_buffer_handler = 
            [&](hermes::request<send_buffer_args>&& req) {

                send_buffer_args args =  req.args();

                auto remote_buffers = args.buffers();

                // 1. allocate local buffers according to info in remote_buffers
                // 2. local_buffers = expose(bufs)
                // 3. hermes::transfer(remote_buffers, local_buffers);

                std::vector<hermes::mutable_buffer> bufvec;
                bufvec.reserve(remote_buffers.count());

                for(auto&& remote_buf : args.buffers()) {
                    std::size_t remote_size = remote_buf.size();
                    char* data = new char[remote_size];
                    bufvec.emplace_back(data, remote_size);
                }

                INFO("RPC received:");
                INFO("    type: send_buffer,"); 
                INFO("    args: remote_buffers{{count={}, total_size={}}}", 
                        remote_buffers.count(), remote_buffers.size());

                // TODO: remove the need to expose local buffers and just pass
                // the bufvec directly to async_pull
                auto local_buffers = 
                    hg.expose(bufvec, hermes::access_mode::write_only);

                INFO("  Pulling remote buffers");

                // this lambda will be invoked when the pull transfer completes
                // (we capture bufvec by reference to verify that remote data
                // was actually copied into the buffers, but it's not necessary
                // in real code)
                const auto do_pull_completion = 
                    [&bufvec, &hg](hermes::request<send_buffer_args>&& req) {

                    INFO("    Pull successful!");

                    for(auto&& local_buf : bufvec) {
                        INFO("      Buffer size: {}", local_buf.size());
                        INFO("      Buffer contents: {}", 
                                reinterpret_cast<char*>(local_buf.data()));
                    }

                    if(req.requires_response()) {
                        hermes::send_buffer_retval rv(42);
                        hg.respond<hermes::rpc::send_buffer>(std::move(req), rv);
                        INFO("  Response sent with value {}", rv.retval());
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
                          ex, std::placeholders::_1));

        hg.run();

        while(true) {
            sleep(1);
        }
    } 
    catch(const std::exception& ex) {
        ERROR("{}", ex.what());
        throw;
    }

    return 0;
}
