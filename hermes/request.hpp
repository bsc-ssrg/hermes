#ifndef __HERMES_REQUEST_HPP__
#define __HERMES_REQUEST_HPP__

#include <mercury.h>

#include "logging.hpp"

namespace hermes {

// forward declarations
namespace detail {

template <typename ExecutionContext, typename Callable>
inline void
bulk_transfer(hermes::request&& req,
              ExecutionContext&& ctx,
              Callable&& completion_callback);

template <typename Output>
inline void
respond(request&& req, Output&& out);

}

class request {

    friend class async_engine;

    template <typename ExecutionContext, typename Callable>
    friend void
    detail::bulk_transfer(hermes::request&& req,
                          ExecutionContext&& ctx,
                          Callable&& completion_callback);

    template <typename Output>
    friend void
    detail::respond(request&& req, Output&& out);

public:

    request(hg_handle_t handle, 
            bool requires_response = true) :
        m_handle(handle),
        m_requires_response(requires_response),
        m_remote_bulk_handle(HG_BULK_NULL),
        m_local_bulk_handle(HG_BULK_NULL) { }

    request(hg_handle_t handle, 
            hg_bulk_t remote_bulk_handle, 
            bool requires_response = true) :
        m_handle(handle),
        m_requires_response(requires_response),
        m_remote_bulk_handle(remote_bulk_handle),
        m_local_bulk_handle(HG_BULK_NULL) { }

    request(const request& other) = delete;
    request(request&& rhs) = default;
    request& operator=(const request& other) = delete;
    request& operator=(request&& rhs) = default;

    bool
    requires_response() const {
        return m_requires_response;
    }

    ~request() { 
        DEBUG2("HG_Destroy({})", fmt::ptr(m_handle));
        HG_Destroy(m_handle);
    }

private:
    hg_handle_t m_handle;
    bool m_requires_response;
    hg_bulk_t m_remote_bulk_handle;
    hg_bulk_t m_local_bulk_handle;
};

} // namespace hermes

#endif // __HERMES_REQUEST_HPP__
