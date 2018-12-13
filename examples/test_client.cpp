#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <hermes.hpp>
#include "rpcs.v2.hpp"

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

    try {

        // initialize the engine with appropriate transport protocol
        async_engine hg(transport::ofi_tcp);

        endpoint_set endps = hg.lookup({
            {"127.0.0.1", 22222},
        });

        // start the asynchronous engine
        hg.run();

#if 1
        /*********************************************************************** 
         * Example 1: posting an RPC with arbitrary arguments and 
         * an expected response
         **********************************************************************/
        const std::string message("Hello world!!!");

        HERMES_INFO("Sending RPC (send_message, args: \"{}\")", message);

        auto rpc1 = hg.broadcast<example_rpcs::send_message>(endps, message);

        // wait for results
        hermes::output_set<example_rpcs::send_message> results = rpc1.get();

        HERMES_INFO("Output received (size: {})", results.size());

        for(auto&& rv : results) {
            HERMES_INFO("retval: {}", rv.retval());
        }
#endif

#if 1
        /*********************************************************************** 
         * Example 2: posting an RPC with arbitrary arguments plus an 
         * additional transfer of associated buffers 
         **********************************************************************/
        __attribute__((unused)) char data[]  = {"These are the contents of a user buffer\0"};

#if 0
        std::vector<hermes::mutable_buffer> bufvec {
            hermes::mutable_buffer{data, sizeof(data)},
        };

#else

        auto pp = std::make_shared<hermes::mapped_buffer>("../examples/rpcs.v2.hpp");

        std::vector<hermes::mutable_buffer> bufvec {
            hermes::mutable_buffer{pp->data(), pp->size()},
        };

        HERMES_INFO("      Initial buffer contents: \"{}\"", 
                std::string(reinterpret_cast<char*>(pp->data()), 150));
#endif

        hermes::exposed_memory exposed_buffers = 
            hg.expose(bufvec, access_mode::read_only);

        HERMES_INFO("Sending [send_buffer] RPC");

        // Option 1: 
        //   Instantiate an [RPC]_args object containing the arguments and
        //   pass it to the post() function
        example_rpcs::send_buffer::input in3a("test3a", exposed_buffers);
        auto rpc2a = hg.broadcast<example_rpcs::send_buffer>(endps, in3a);

        HERMES_INFO("Sending [send_buffer] RPC");

        // Option 2: 
        //   Instantiate an [RPC]_args object containing the arguments 
        //   directly as a temporary in the post() function
        auto rpc2b = 
            hg.broadcast<example_rpcs::send_buffer>(
                    endps, example_rpcs::send_buffer::input(
                            "test3b", exposed_buffers));

        HERMES_INFO("Sending [send_buffer] RPC");

        // Option 3: 
        //   Pass the RPC arguments directly to the post() function
        auto rpc2c = hg.broadcast<example_rpcs::send_buffer>(
                        endps, "test3c", exposed_buffers);

        // wait for results
        hermes::output_set<example_rpcs::send_buffer> results2a = rpc2a.get();
        // hermes::output_set<example_rpcs::send_buffer> results2b = rpc2b.get();
        // hermes::output_set<example_rpcs::send_buffer> results2c = rpc2c.get();

        HERMES_INFO("result_set size {}:", results2a.size());

        for(auto&& rv : results2a) {
            HERMES_INFO("{}", rv.retval());
        }
#endif

        HERMES_INFO("Sending [shutdown]");

        hg.post<example_rpcs::shutdown>(endps[0]);
    } 
    catch(const std::exception& ex) {
        ERROR("{}\n", ex.what());
        //throw;
    }

    return 0;
}
