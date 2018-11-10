#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "rpcs.hpp"
#include "hermes.hpp"

struct mapped_buffer {

    explicit mapped_buffer(const std::string& pathname) {

        int fd = ::open(pathname.c_str(), O_RDONLY);

        if(fd == -1) {
            throw std::runtime_error("XXX");
        }

        struct stat stbuf;

        if(::fstat(fd, &stbuf) == -1) {
            throw std::runtime_error("XXX");
        }

        m_size = stbuf.st_size;


        m_data = ::mmap(NULL, 
                        stbuf.st_size, 
                        PROT_READ, MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB,
                        fd,
                        0);

        ::close(fd);
    }

    ~mapped_buffer() {
        if(m_data != NULL) {
            munmap(m_data, m_size);
        }
    }

    void*
    data() const {
        return m_data;
    }

    std::size_t
    size() const {
        return m_size;
    }

    void* m_data;
    std::size_t m_size;
};


int
main(int argc, char* argv[]) {

    (void) argc;
    (void) argv;

    using hermes::transport;
    using hermes::async_engine;
    using hermes::endpoint_set;
    using hermes::result_set;
    using hermes::rpc;
    using hermes::access_mode;
    using hermes::send_buffer_args;

    try {

        // initialize the engine with appropriate transport protocol
        async_engine hg(transport::ofi_tcp);

        endpoint_set endps = hg.lookup({
            {"127.0.0.1", 22222},
        });

//        mapped_buffer buf("rpcs.hpp");

        // start the asynchronous engine
        hg.run();

        /*********************************************************************** 
         * Example 1: posting an RPC with arbitrary arguments and 
         * an expected response
         **********************************************************************/
        const std::string message("Hello world!!!");

        INFO("Sending RPC [send_message, args: \"{}\"]", message);

        auto rpc1 = 
            hg.post<rpc::send_message>(endps, message);

        // wait for results
        result_set<rpc::send_message> results = rpc1.get();

        INFO("All results received [size: {}]", results.size());

        for(auto&& rv : results) {
            INFO("retval: {}", rv.retval());
        }

        return 0;

        /*********************************************************************** 
         * Example 2: posting an RPC with arbitrary arguments plus an 
         * additional transfer of associated buffers 
         **********************************************************************/
        char data[] = {"These are the contents of a user buffer\0"};

        std::vector<hermes::mutable_buffer> bufvec {
            hermes::mutable_buffer{data, sizeof(data)}
        };

        auto exposed_buffers = 
            hg.expose(bufvec, access_mode::read_only);

        INFO("Sending [send_buffer] RPC");

        // Option 1: 
        //   Instantiate an [RPC]_args object containing the arguments and
        //   pass it to the post() function
        hermes::send_buffer_args in3a("test3a", exposed_buffers);
        auto rpc2a = 
            hg.post<rpc::send_buffer>(endps, in3a);

        // Option 2: 
        //   Instantiate an [RPC]_args object containing the arguments 
        //   directly as a temporary in the post() function
        auto rpc2b = 
            hg.post<rpc::send_buffer>(endps, 
                    send_buffer_args("test3b", exposed_buffers));

        // Option 3: 
        //   Pass the RPC arguments directly to the post() function
        auto rpc2c = 
            hg.post<rpc::send_buffer>(endps, "test3c", exposed_buffers);

        // wait for results
        result_set<rpc::send_buffer> results2a = rpc2a.get();
        result_set<rpc::send_buffer> results2b = rpc2b.get();
        result_set<rpc::send_buffer> results2c = rpc2c.get();

        INFO("result_set size {}:", results2a.size());

        for(auto&& rv : results2a) {
            INFO("{}", rv.retval());
        }
    } 
    catch(const std::exception& ex) {
        ERROR("{}\n", ex.what());
        throw;
    }

    return 0;
}
