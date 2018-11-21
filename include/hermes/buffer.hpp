#ifndef __HERMES_BUFFER_HPP__
#define __HERMES_BUFFER_HPP__

// C includes
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

// C++ includes
#include <cstddef>
#include <stdexcept>

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
    explicit mapped_buffer(const std::string& pathname) {

        const auto build_error_msg = [](const std::string& msg) {
            // 1024 should be more than enough for most locales
            char buffer[1024];
            return msg + ": " +
                std::string(::strerror_r(errno, buffer, sizeof(buffer)));
        };

        int fd = ::open(pathname.c_str(), O_RDONLY);

        if(fd == -1) {
            throw std::runtime_error(
                    build_error_msg("Failed to open file"));
        }

        struct stat stbuf;

        if(::fstat(fd, &stbuf) != 0) {
            throw std::runtime_error(
                    build_error_msg("Failed to retrieve file size"));
        }

        m_size = stbuf.st_size;

        int flags = MAP_SHARED;

#ifdef MAP_HUGETLB
        flags |= MAP_HUGETLB;
#endif

        m_data = ::mmap(NULL, stbuf.st_size, PROT_READ, flags, fd, 0);

        if(m_data == MAP_FAILED) {
#ifdef MAP_HUGETLB
            // the system may not have huge pages configured, or we may have 
            // exhausted them, retry with normal-size pages
            flags &= ~(MAP_HUGETLB);
            m_data = ::mmap(NULL, stbuf.st_size, PROT_READ, flags, fd, 0);

            if(m_data == MAP_FAILED) {
#endif
                throw std::runtime_error(
                        build_error_msg("mmap() on file failed"));
#ifdef MAP_HUGETLB
            }
#endif
        }

        if(::close(fd) != 0) {
            throw std::runtime_error(
                    build_error_msg("close() on file failed"));
        }
    }

    ~mapped_buffer() {
        if(m_data != NULL) {
            // don't bother checking the error since we can't report it
            ::munmap(m_data, m_size);
        }
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

private:
    void* m_data;
    std::size_t m_size;
};

} // namespace hermes

#endif // __HERMES_BUFFER_HPP__
