#ifndef __HERMES_BUFFER_HPP__
#define __HERMES_BUFFER_HPP__

#include <cstddef>

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

    /** */
    void
    reset(void* data, std::size_t size) {
        m_data = data;
        m_size = size;
    }

private:
    void* m_data;
    std::size_t m_size;
};

} // namespace hermes

#endif // __HERMES_BUFFER_HPP__
