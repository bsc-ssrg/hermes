#ifndef __HERMES_BUFFER_HPP__
#define __HERMES_BUFFER_HPP__

// C includes
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

// C++ includes
#include <cstddef>
#include <stdexcept>
#include <system_error>

// project includes
#include <hermes/access_mode.hpp>
#include <hermes/logging.hpp>

namespace {

constexpr int 
get_page_protections(hermes::access_mode mode) {
    using hermes::access_mode;
    return (mode == access_mode::read_only ? PROT_READ : 
               (mode == access_mode::write_only ? PROT_WRITE :
                   (mode == access_mode::read_write ? (PROT_READ | PROT_WRITE) : 
                   PROT_NONE)
               )
           );
}

} // anonymous namespace

namespace hermes {

/**
 * The mutable_buffer class provides a safe representation of a buffer that can
 * be modified. It does not own the underlying data, and so is cheap to copy or
 * assign.
 *
 * @par Accessing Buffer Contents
 *
 * The contents of a buffer may be accessed using the @c data() and @c size()
 * member functions:
 *
 * @code hermes::mutable_buffer b1 = ...;
 * std::size_t s1 = b1.size();
 * unsigned char* p1 = static_cast<unsigned char*>(b1.data());
 * @endcode
 *
 * The @c data() member function permits violations of type safety, so uses of
 * it in application code should be carefully considered.
 */
class mutable_buffer {

public:
    /** Constructs an empty buffer */
    mutable_buffer() :
        m_data(),
        m_size() { }

    /** Constructs a buffer to represent a memory region */
    mutable_buffer(void* data, std::size_t size) :
        m_data(data),
        m_size(size) { }

    /** Returns a pointer to the beginning of the memory region */
    void*
    data() const {
        return m_data;
    }

    /** Returns the size of the memory region */
    std::size_t
    size() const {
        return m_size;
    }

private:
    void* m_data;
    std::size_t m_size;
};


// TODO
class mapped_buffer {

public:
    mapped_buffer(const std::string& pathname,
                  hermes::access_mode access_mode,
                  std::error_code* ec = 0) {

        const auto build_error_msg = [](const std::string& msg) {
            // 1024 should be more than enough for most locales
            char buffer[1024];
            return msg + ": " +
                std::string(::strerror_r(errno, buffer, sizeof(buffer)));
        };

        int fd = ::open(pathname.c_str(), O_RDWR);

        if(fd == -1) {
            if(ec == 0) {
                throw std::runtime_error(
                        build_error_msg("Failed to open file"));
            }

            *ec = std::make_error_code(static_cast<std::errc>(errno));
            return;
        }

        struct stat stbuf;

        if(::fstat(fd, &stbuf) != 0) {
            if(ec == 0) {
                throw std::runtime_error(
                        build_error_msg("Failed to retrieve file size"));
            }

            *ec = std::make_error_code(static_cast<std::errc>(errno));
            ::close(fd);
            return;
        }

        m_size = stbuf.st_size;

        int prots = ::get_page_protections(access_mode);
        int flags = MAP_SHARED;

#ifdef MAP_HUGETLB
        flags |= MAP_HUGETLB;
#endif

        m_data = ::mmap(NULL, stbuf.st_size, prots, flags, fd, 0);

        if(m_data == MAP_FAILED) {
            HERMES_DEBUG2("::mmap(NULL, {}, {:#x}, {:#x}, {}, 0) = MAP_FAILED", 
                          stbuf.st_size, prots, flags, fd, 0);

#ifdef MAP_HUGETLB
            // the system may not have huge pages configured, or we may have 
            // exhausted them, retry with normal-size pages
            flags &= ~(MAP_HUGETLB);
            m_data = ::mmap(NULL, stbuf.st_size, prots, flags, fd, 0);

            if(m_data == MAP_FAILED) {
                HERMES_DEBUG2("::mmap(NULL, {}, {:#x}, {:#x}, {}, 0) = MAP_FAILED", 
                              stbuf.st_size, prots, flags, fd, 0);
#endif
                if(ec == 0) {
                    throw std::runtime_error(
                            build_error_msg("mmap() on file failed"));
                }

                *ec = std::make_error_code(static_cast<std::errc>(errno));
                ::close(fd);
                return;
#ifdef MAP_HUGETLB
            }
#endif
        }
        
        HERMES_DEBUG2("::mmap(NULL, {}, {:#x}, {:#x}, {}, 0) = {}", 
                      stbuf.st_size, prots, flags, fd, m_data);

        if(::close(fd) != 0) {

            if(ec == 0) {
                throw std::runtime_error(
                        build_error_msg("close() on file failed"));
            }

            *ec = std::make_error_code(static_cast<std::errc>(errno));
        }

        m_access_mode = access_mode;
    }

    mapped_buffer(const mapped_buffer& other) = delete;
    mapped_buffer& operator=(const mapped_buffer& other) = delete;

    ~mapped_buffer() {
        this->unmap();
    }

    /** Returns a pointer to the beginning of the memory region */
    void*
    data() const {
        return m_data;
    }

    /** Returns the size of the memory region */
    std::size_t
    size() const {
        return m_size;
    }

    std::tuple<void*, std::size_t>
    release() {
        auto ret = std::make_tuple(m_data, m_size);
        m_data = NULL;
        m_size = 0;
        return ret;
    }

    hermes::access_mode
    access_mode() const {
        return m_access_mode;
    }

    void
    protect(hermes::access_mode access_mode,
            std::error_code* ec = 0) {

        const auto build_error_msg = [](const std::string& msg) {
            // 1024 should be more than enough for most locales
            char buffer[1024];
            return msg + ": " +
                std::string(::strerror_r(errno, buffer, sizeof(buffer)));
        };

        if(::mprotect(m_data, m_size, 
                      ::get_page_protections(access_mode)) != 0) {

            std::error_code error;
            error.assign(errno, std::generic_category());

            HERMES_DEBUG2("::mprotect({}, {}, {:#x}) = {}", 
                          m_data, m_size, ::get_page_protections(access_mode), 
                          error.message());

            if(ec == 0) {
                throw std::runtime_error(
                        build_error_msg("mprotect() failed"));
            }

            *ec = error;
            return;
        }

        m_access_mode = access_mode;
    }

    void 
    unmap() {
        if(m_data != NULL) {
            // don't bother checking the error since we can't report it
            int rv = ::munmap(m_data, m_size);

            if(rv != 0) {
                HERMES_ERROR("::munmap({}, {}) = {} (errno: {})", 
                             m_data, m_size, rv, errno);
            }

            HERMES_DEBUG2("::munmap({}, {}) = {}", m_data, m_size, rv);
        }

        m_data = NULL;
        m_size = 0;
    }

private:
    void* m_data;
    std::size_t m_size;
    hermes::access_mode m_access_mode;
};

} // namespace hermes

#endif // __HERMES_BUFFER_HPP__
