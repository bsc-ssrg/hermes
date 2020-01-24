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

struct message_printer {

    message_printer(const hermes::async_engine& hg) :
        m_hg(hg) { }

    const int m_retval = 36;

    void
    handler(hermes::request<example_rpcs::send_message>&& req) {

        example_rpcs::send_message::input args = req.args();

        std::cout << "RPC received:\n";
        std::cout << "    type: send_message,\n"; 
        std::cout << "    args: \"" << args.message() << "\"\n";

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
            m_hg.respond<example_rpcs::send_message>(std::move(req), m_retval);
            std::cout << "  Response sent with value " <<  m_retval << "\n";
        }
    }

    const hermes::async_engine& m_hg;
};

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

        message_printer ex(hg);

        // register handlers for any defined rpcs
        hg.register_handler<example_rpcs::send_message>(
                std::bind(&message_printer::handler, 
                          ex, std::placeholders::_1));

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
        std::cerr << "ERROR: " << ex.what() << "\n";
    }

    return 0;
}
