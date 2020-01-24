#include <vector>
#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

    using hermes::transport;
    using hermes::async_engine;
    using hermes::endpoint_set;
    using hermes::output_set;
    //using hermes::rpc;
    using hermes::access_mode;
    //using hermes::send_buffer_args;

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

        // define and expose any buffers that we want transferred 
        // so that they can be accessed remotely through RMA
        char data[]  = {"These are the contents of an example buffer"};
        auto mapped_file = 
            std::make_shared<hermes::mapped_buffer>(
                    "examples/01_send_buffer/lipsum.txt",
                    hermes::access_mode::read_only);

        std::vector<hermes::mutable_buffer> bufvec {
            hermes::mutable_buffer{data, sizeof(data)},
            hermes::mutable_buffer{mapped_file->data(), mapped_file->size()},
        };

        hermes::exposed_memory exposed_buffers = 
            hg.expose(bufvec, access_mode::read_only);


        std::cout << "Sending RPC (send_buffer)\n";

        // Option 1: 
        //   Instantiate an [RPC]::input object containing the arguments and
        //   pass it to the post() function
        example_rpcs::send_buffer::input in3a("test3a", exposed_buffers);
        auto rpc2a = hg.post<example_rpcs::send_buffer>(endp, in3a);


        std::cout << "Sending RPC (send_buffer)\n";

        // Option 2: 
        //   Instantiate an [RPC]::input object containing the arguments 
        //   directly as a temporary in the post() function
        auto rpc2b = 
            hg.post<example_rpcs::send_buffer>(
                    endp, example_rpcs::send_buffer::input(
                            "test3b", exposed_buffers));


        std::cout << "Sending RPC (send_buffer)\n";

        // Option 3: 
        //   Pass the RPC arguments directly to the post() function
        auto rpc2c = hg.post<example_rpcs::send_buffer>(
                        endp, "test3c", exposed_buffers);


        std::cout << "Waiting for RPC outputs...\n";

        // wait for results
        hermes::output_set<example_rpcs::send_buffer> results2a = rpc2a.get();
        hermes::output_set<example_rpcs::send_buffer> results2b = rpc2b.get();
        hermes::output_set<example_rpcs::send_buffer> results2c = rpc2c.get();

        std::cout << "Output received (size: " << results2a.size() << ")\n";
        std::cout << "Output received (size: " << results2b.size() << ")\n";
        std::cout << "Output received (size: " << results2c.size() << ")\n";

        for(auto&& rv : results2a) {
            std::cout << "retval: " << rv.retval() << "\n";
        }

        for(auto&& rv : results2b) {
            std::cout << "retval: " << rv.retval() << "\n";
        }

        for(auto&& rv : results2c) {
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
