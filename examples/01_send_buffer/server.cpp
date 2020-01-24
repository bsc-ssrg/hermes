#include <unistd.h>

#include <vector>
#include <string>
#include <iostream>

#include <hermes.hpp>

#include "rpcs.hpp"
#include "common.hpp"

std::atomic<bool> shutdown_requested(false);

void
shutdown_handler(hermes::request<example_rpcs::shutdown>&& req) {

    std::cout << "RPC received:\n";
    std::cout << "    type: shutdown\n";
    std::cout << "  requires_response? " 
              << std::boolalpha << req.requires_response() << "\n"; 

    bool expected = false;
    while(!shutdown_requested.compare_exchange_weak(expected, true) 
            && !expected);
}

std::tuple<hermes::transport, std::string>
parse_args(int argc, char* argv[]) {

    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " ADDRESS\n";

        std::cerr << 
            "Address formats:\n"                                           <<
            "  [BMI plugin]\n"                                             <<
            "    tcp:      bmi+tcp[://<hostname,IP>:<port>]\n\n"           <<
            "  [MPI plugin]\n"                                             <<
            "    static:   mpi+static\n"                                   <<
            "    dynamic:  mpi+dynamic\n\n"                                <<
            "  [SM plugin]\n"                                              <<
            "    sm:       na+sm\n\n"                                      <<
            "  [OFI/libfabric plugin]\n"                                   <<
            "    tcp:      ofi+tcp[://<hostname,IP,interface>:<port>]\n"   <<
            "    verbs:    ofi+verbs[://<hostname,IP,interface>:<port>]\n" <<
            "    psm2:     ofi+psm2\n"                                     <<
            "    gni:      ofi+gni[://<hostname,IP,interface>]\n\n"        <<
            "  [CCI (deprecated)]\n"                                       <<
            "    tcp:      cci+tcp[://<hostname,IP,interface>:<port>]\n"   <<
            "    verbs:    cci+verbs[://<hostname,IP,interface>:<port>]\n" <<
            "    sm:       cci+sm[://<PID>/<ID>]\n";

        exit(1);
    }

    const std::string address(argv[1]);

    std::size_t pos = address.find("://");

    if(pos == std::string::npos) {
        std::cout << "WARNING: Address does not include a transport prefix. "
                     "Defaulting to ofi+tcp\n";

        return std::make_tuple(hermes::transport::ofi_tcp, address);
    }
    else {
        return std::make_tuple(
                hermes::get_transport_type(address.substr(0, pos)),
                address.substr(pos+3));
    }
}

int
main(int argc, char* argv[]) {

#ifdef HERMES_ENABLE_LOGGING
    hermes::log::logger::register_callback(hermes::log::info, common::log_info);
    hermes::log::logger::register_callback(hermes::log::warning, common::log_warning);
    hermes::log::logger::register_callback(hermes::log::error, common::log_error);
    hermes::log::logger::register_callback(hermes::log::fatal, common::log_fatal);

#ifdef HERMES_DEBUG_BUILD
    hermes::log::logger::register_callback(hermes::log::debug, common::log_debug);
#endif // HERMES_DEBUG_BUILD

    hermes::log::logger::register_callback(hermes::log::mercury, common::log_mercury);
#endif // HERMES_ENABLE_LOGGING

    try {

        hermes::transport tr;
        std::string bind_address;

        std::tie(tr, bind_address) = parse_args(argc, argv);

        // initialize the engine with the provided address
        hermes::async_engine hg(tr, bind_address, true);

        // define and register handlers for any defined rpcs
        const auto send_buffer_handler = 
            [&](hermes::request<example_rpcs::send_buffer>&& req) {

                example_rpcs::send_buffer::input args = req.args();

                hermes::exposed_memory remote_buffers = args.buffers();

                std::cout << "RPC received:\n";
                std::cout << "    type: send_buffer,\n"; 
                std::cout << "    args: remote_buffers{count="
                          << remote_buffers.count() 
                          << ", total_size=" << remote_buffers.size() << " }\n"; 

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

                //sleep(120);

                std::cout << "  Pulling remote buffers\n";

                // this lambda will be invoked when the pull transfer completes
                // (we capture bufvec by reference to verify that remote data
                // was actually copied into the buffers, but it's not necessary
                // in real code)
                const auto do_pull_completion = [bufseq, &hg](
                        hermes::request<example_rpcs::send_buffer>&& req) {

                    std::cout << "    Pull successful!\n";

                    for(auto&& buf : bufseq) {
                        std::cout << "     Buffer size: " << buf.size() << "\n";
                        std::cout << "     Initial buffer contents:\n" 
                                  << ">>>> BUFFER START <<<<\n"
                                  << std::string(
                                          reinterpret_cast<char*>(
                                              buf.data()), 
                                          std::min(buf.size(), 250ul))
                                  << "\n>>>> BUFFER END <<<<\n";
                    }

                    if(req.requires_response()) {

                        std::cout << "  Sending response...\n";

                        example_rpcs::send_buffer::output rv(42);
                        hg.respond<example_rpcs::send_buffer>(
                                std::move(req), rv);

                        std::cout << "  Response sent with value " 
                                  << rv.retval() << "\n";
                    }
                };

                hg.async_pull(remote_buffers,
                              local_buffers,
                              std::move(req),
                              do_pull_completion);
            };

        hg.register_handler<example_rpcs::send_buffer>(send_buffer_handler);

        hg.register_handler<example_rpcs::shutdown>(shutdown_handler);

        std::cout << "Listening for requests\n";

        // start the engine
        hg.run();

        while(!shutdown_requested) {
            // the server could do actual useful work here while the
            // engine processes rpcs and invokes handlers
            sleep(1);
        }

        std::cout << "Shutting down\n";
    } 
    catch(const std::exception& ex) {
        std::cerr << ex.what() << "\n";
    }

    return 0;
}
