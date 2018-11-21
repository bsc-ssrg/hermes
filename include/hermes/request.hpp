#ifndef __HERMES_REQUEST_HPP__
#define __HERMES_REQUEST_HPP__

#include "logging.hpp"

namespace hermes {

// forward declarations

// defined in this file
template <typename Request>
class request;

namespace detail {

// defined elsewhere
template <typename Input, typename ExecutionContext, typename Callable>
inline void
mercury_bulk_transfer(hermes::request<Input>&& req,
                      ExecutionContext&& ctx,
                      Callable&& completion_callback);

// defined elsewhere
template <typename Input, typename Output>
inline void
mercury_respond(request<Input>&& req, Output&& out);

}

template <typename Request>
class request {

    friend class async_engine;

    using Input = typename Request::input_type;
    using MercuryInput = typename Request::mercury_input_type;

    template <typename RequestInput, 
              typename ExecutionContext, 
              typename Callable>
    friend void
    detail::mercury_bulk_transfer(hermes::request<RequestInput>&& req,
                                  ExecutionContext&& ctx,
                                  Callable&& completion_callback);

    template <typename RequestInput, 
              typename Output>
    friend void
    detail::mercury_respond(request<RequestInput>&& req, Output&& out);

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
        m_requires_response(requires_response) { }

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
        DEBUG2("{}()", __func__);

        if(m_handle != HG_HANDLE_NULL) {
            DEBUG2("*********************** HG_Destroy({})", fmt::ptr(m_handle));
            HG_Destroy(m_handle);
        }
    }

    //TODO: make private
private:
    hg_handle_t m_handle;
    bool m_requires_response;
    Input m_args;
};



} // namespace hermes

#endif // __HERMES_REQUEST_HPP__
