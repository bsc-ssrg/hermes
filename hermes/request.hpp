#ifndef __HERMES_REQUEST_HPP__
#define __HERMES_REQUEST_HPP__

#include <mercury.h>

namespace hermes {

class exposed_memory;

class request {

    template <typename Callable>
    friend void pull(const request&, 
                     const exposed_memory&, 
                     const exposed_memory&, 
                     Callable&&);
    friend class async_engine;

public:

    request(hg_handle_t handle) :
        m_handle(handle),
        m_bulk_handle(HG_BULK_NULL) { }

    request(hg_handle_t handle, hg_bulk_t bulk_handle) :
        m_handle(handle),
        m_bulk_handle(bulk_handle) { }

    ~request() { 
        /// XXX free handle?
    }

private:
    hg_handle_t m_handle;
    hg_bulk_t m_bulk_handle;
};

} // namespace hermes

#endif // __HERMES_REQUEST_HPP__
