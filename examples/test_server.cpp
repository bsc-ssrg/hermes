#include <vector>
#include <unistd.h>
#include <iostream>

#include <hermes.hpp>
#include "rpcs.v2.hpp"

#if 0
using hermes::request;
using hermes::async_engine;
using hermes::rpc;
using hermes::send_file_args;
using hermes::send_message_args;
using hermes::send_buffer_args;
#endif

std::atomic<bool> shutdown_requested(false);

void
shutdown_handler(hermes::request<example_rpcs::shutdown>&& req) {

    HERMES_INFO("RPC received:");
    HERMES_INFO("    type: shutdown"); 
    HERMES_INFO("    requires_response?: {}", req.requires_response()); 

    bool expected = false;
    while(!shutdown_requested.compare_exchange_weak(expected, true) 
            && !expected);
}

void
send_file_handler(hermes::request<example_rpcs::send_file>&& req) {

    example_rpcs::send_file::input args = req.args();

    HERMES_INFO("RPC received:");
    HERMES_INFO("    type: send_message,"); 
    HERMES_INFO("    args: \"{}\"", args.pathname());

    // hermes::send_file_out_t out;
    // out.ret_val = 42;

    HERMES_INFO("Callback invoked: {}(\"{}\") = {}!", 
         __FUNCTION__, args.pathname(), "?");

#if 0
    if(req.requires_response()) {
        m_hg.respond<example_rpcs::send_file>(std::move(req), 0);
        HERMES_INFO("  Response sent with value {}", 0);
    }
#endif
}

struct example_class {

    example_class(const hermes::async_engine& hg) :
        m_hg(hg) { }

    const int m_retval = 36;

    void
    handler(hermes::request<example_rpcs::send_message>&& req) {

        example_rpcs::send_message::input args = req.args();

        HERMES_INFO("RPC received:");
        HERMES_INFO("    type: send_message,"); 
        HERMES_INFO("    args: \"{}\"", args.message());

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
            m_hg.respond<example_rpcs::send_message>(std::move(req), 0);
            HERMES_INFO("  Response sent with value {}", 0);
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
        async_engine hg(hermes::transport::ofi_tcp, true);

        const auto send_buffer_handler = 
            [&](hermes::request<example_rpcs::send_buffer>&& req) {

                example_rpcs::send_buffer::input args = req.args();

                hermes::exposed_memory remote_buffers = args.buffers();

                HERMES_INFO("RPC received:");
                HERMES_INFO("    type: send_buffer,"); 
                HERMES_INFO("    args: remote_buffers{{count={}, total_size={}}}", 
                        remote_buffers.count(), remote_buffers.size());

                // let's prepare some local buffers
                std::vector<hermes::mutable_buffer> bufseq;
                bufseq.reserve(remote_buffers.count());

                for(auto&& rbuf : remote_buffers) {
                    std::size_t remote_size = rbuf.size();
                    char* data = new char[remote_size];
                    bufseq.emplace_back(data, remote_size);
                }

                hermes::exposed_memory local_buffers =
                    hg.expose(bufseq, hermes::access_mode::write_only);

                HERMES_INFO("  Pulling remote buffers");

                // this lambda will be invoked when the pull transfer completes
                // (we capture bufvec by reference to verify that remote data
                // was actually copied into the buffers, but it's not necessary
                // in real code)
                const auto do_pull_completion = [bufseq, &hg](
                        hermes::request<example_rpcs::send_buffer>&& req) {

                    HERMES_INFO("    Pull successful!");

                    for(auto&& buf : bufseq) {
                        HERMES_INFO("      Buffer size: {}", buf.size());
                        // HERMES_INFO("      Buffer contents: {}", 
                        //         reinterpret_cast<char*>(buf.data()));
                        HERMES_INFO("      Initial buffer contents: \"{}\"", 
                                std::string(reinterpret_cast<char*>(buf.data()), 50));
                    }

                    if(req.requires_response()) {
                        HERMES_INFO("  Sending response...");
                        example_rpcs::send_buffer::output rv(42);
                        hg.respond<example_rpcs::send_buffer>(std::move(req), rv);
                        HERMES_INFO("  Response sent with value {}", rv.retval());
                    }
                };

                hg.async_pull(remote_buffers,
                              local_buffers,
                              std::move(req),
                              do_pull_completion);
            };

        hg.register_handler<example_rpcs::send_buffer>(send_buffer_handler);

        hg.register_handler<example_rpcs::send_file>(send_file_handler);

        hg.register_handler<example_rpcs::shutdown>(shutdown_handler);

        example_class ex(hg);

        hg.register_handler<example_rpcs::send_message>(
                std::bind(&example_class::handler, 
                          ex, std::placeholders::_1));

        hg.run();

        while(!shutdown_requested) {
            sleep(1);
        }

        HERMES_INFO("Shutting down");
    } 
    catch(const std::exception& ex) {
        ERROR("{}", ex.what());
        throw;
    }

    return 0;
}
