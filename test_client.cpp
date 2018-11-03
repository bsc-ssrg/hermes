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

    try {

        hermes::async_engine hg(hermes::transport_type::bmi_tcp);

        hermes::endpoint_set endps = hg.lookup({
            {"localhost", 22222},
        });

        mapped_buffer buf("rpcs.hpp");

        hg.run();

        // prepare input data for RPCs
        // hermes::send_message_in_t in1;
        // in1.message = "Hello world!";

        hermes::send_message_args in1("Hello world!!!");

        // hermes::send_file_in_t in2;
        // in2.pathname = "./foobar";

        // hermes::send_buffer_in_t in3;

        // std::vector<hermes::mutable_buffer> bufvec{
        //     {buf.data(), buf.size()}
        // };

        char data[] = {"This is a user buffer\0"};

        std::vector<hermes::mutable_buffer> bufvec{
            hermes::mutable_buffer{data, sizeof(data)}
        };

        auto exposed_buffers = hg.expose(bufvec, hermes::access_mode::read_only);

        hermes::send_buffer_args in3b("test", exposed_buffers);
        //in3.buffer_address = reinterpret_cast<hg_uint64_t>(buf.data());
        //in3.buffer_size = reinterpret_cast<hg_uint64_t>(buf.size());

        // create RPCs
        auto h1 = 
            hg.make_rpc<hermes::rpc::send_message>(endps, in1);

        // auto h2 = 
        //     hg.make_rpc<hermes::rpc::send_file>(endps, in2, 0xdeadbeef);


        auto h3 = 
            hg.make_rpc<hermes::rpc::send_buffer>(endps, in3b);

        // submit RPCs
        hg.submit(h1);
        //hg.submit(h2);
        hg.submit(h3);


//
        sleep(5);

    } 
    catch(const std::exception& ex) {
        throw;
        FATAL("{}\n", ex.what());
    }

    return 0;
}
