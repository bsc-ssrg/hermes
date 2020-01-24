#include <vector>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <hermes.hpp>

#include "rpcs.hpp"
#include "common.hpp"

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

    (void) argc;
    (void) argv;

    using hermes::transport;
    using hermes::async_engine;
    using hermes::endpoint_set;
    using hermes::output_set;
    //using hermes::rpc;
    using hermes::access_mode;
    //using hermes::send_buffer_args;
    //

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

    hermes::transport tr;
    std::string target_address;

    std::tie(tr, target_address) = parse_args(argc, argv);

    try {

        // initialize the engine with appropriate transport protocol
        async_engine hg(transport::ofi_tcp);

        hermes::endpoint endp = hg.lookup(target_address);

        // start the asynchronous engine
        hg.run();

        const std::string message("Hello world!!!");

        std::cout << "Sending RPC (send_message, args: \"" 
                  << message << "\")\n";

        auto rpc1 = hg.post<example_rpcs::send_message>(endp, message);

        std::cout << "Waiting for RPC output...\n";

        // wait for results
        hermes::output_set<example_rpcs::send_message> results = rpc1.get();

        std::cout << "Output received (size: " << results.size() << ")\n";

        for(auto&& rv : results) {
            std::cout << "retval: " << rv.retval() << "\n";
        }

        std::cout << "Sending [shutdown]\n";

        hg.post<example_rpcs::shutdown>(endp);
    } 
    catch(const std::exception& ex) {
        std::cerr << ex.what() << "\n";
    }

    return 0;
}
