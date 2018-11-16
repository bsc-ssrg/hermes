#ifndef __HERMES_REQUEST_HPP__
#define __HERMES_REQUEST_HPP__

#include <mercury.h>

#include "logging.hpp"

namespace hermes {

// forward declarations

// defined in this file
template <typename Input>
class request;

// defined elsewhere
namespace detail {

template <typename Input, typename ExecutionContext, typename Callable>
inline void
bulk_transfer(hermes::request<Input>&& req,
              ExecutionContext&& ctx,
              Callable&& completion_callback);

template <typename Input, typename Output>
inline void
respond(request<Input>&& req, Output&& out);

}

template <typename Request>
class request {

    friend class async_engine;

    using Input = typename Request::input_type;

    template <typename RequestInput, 
              typename ExecutionContext, 
              typename Callable>
    friend void
    detail::bulk_transfer(hermes::request<RequestInput>&& req,
                          ExecutionContext&& ctx,
                          Callable&& completion_callback);

    template <typename RequestInput, 
              typename Output>
    friend void
    detail::respond(request<RequestInput>&& req, Output&& out);

// TODO: move this 'public' after ctors
public:

    template <typename RequestInput, 
              typename Enable = 
                  typename std::is_same<Input, RequestInput>::type>
    request(hg_handle_t handle, 
            RequestInput&& args,
            bool requires_response = true) :
        m_handle(handle),
        m_args(std::forward<RequestInput>(args)),
        m_requires_response(requires_response),
        m_remote_bulk_handle(HG_BULK_NULL),
        m_local_bulk_handle(HG_BULK_NULL) { }

    template <typename RequestInput, 
              typename Enable = 
                  typename std::is_same<Input, RequestInput>::type>
    request(hg_handle_t handle, 
            hg_bulk_t remote_bulk_handle, 
            RequestInput&& args,
            bool requires_response = true) :
        m_handle(handle),
        m_args(std::forward<RequestInput>(args)),
        m_requires_response(requires_response),
        m_remote_bulk_handle(remote_bulk_handle),
        m_local_bulk_handle(HG_BULK_NULL) { }

    request(const request& other) = delete;
    request(request&& rhs) = default;
    request& operator=(const request& other) = delete;
    request& operator=(request&& rhs) = default;

    Input args() const {
        return m_args;
    }

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

    Input m_args;


};



} // namespace hermes

#endif // __HERMES_REQUEST_HPP__
