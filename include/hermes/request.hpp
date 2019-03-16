#ifndef __HERMES_REQUEST_HPP__
#define __HERMES_REQUEST_HPP__

#include <memory>

// project includes
#if __cplusplus == 201103L
#include <hermes/make_unique.hpp>
#endif // __cplusplus == 201103L

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

template <typename Request>
static inline typename Request::mercury_input_type
decode_mercury_input(hg_handle_t handle);

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

    request(hg_handle_t handle) :
        m_handle(handle),
        m_mercury_input(compat::make_unique<MercuryInput>(
                    detail::decode_mercury_input<Request>(m_handle))),
        m_input(compat::make_unique<Input>(*m_mercury_input)),
        m_requires_response(Request::requires_response) { }

    request(const request& other) = delete;

    request(request&& rhs) :
        m_handle(std::move(rhs.m_handle)),
        m_mercury_input(std::move(rhs.m_mercury_input)),
        m_input(std::move(rhs.m_input)),
        m_requires_response(std::move(rhs.m_requires_response)) { 

        rhs.m_handle = HG_HANDLE_NULL;
        rhs.m_requires_response = false;
    }

    request& operator=(const request& other) = delete;

    request& 
    operator=(request&& rhs) {

        if(this != &rhs) {
            m_handle = std::move(rhs.m_handle);
            m_mercury_input = std::move(rhs.m_mercury_input);
            m_input = std::move(rhs.m_input);
            m_requires_response = std::move(rhs.m_requires_response);

            rhs.m_handle = HG_HANDLE_NULL;
            rhs.m_requires_response = false;
        }

        return *this;
    }

    Input args() const {
        return *m_input;
    }

    bool
    requires_response() const {
        return m_requires_response;
    }

    ~request() { 
        HERMES_DEBUG2("{}(this={})", __func__, fmt::ptr(this));
#if 0
        HERMES_DEBUG2("this = {");
        HERMES_DEBUG2("  m_handle = {},", fmt::ptr(m_handle));
        HERMES_DEBUG2("  m_mercury_input = {},", 
                      fmt::ptr(m_mercury_input.get()));
        HERMES_DEBUG2("  m_input = {},", 
                      fmt::ptr(m_input.get()));
        HERMES_DEBUG2("  m_requires_response = {}", m_requires_response);
        HERMES_DEBUG2("};");
        HERMES_DEBUG_FLUSH();
#endif


        if(m_handle != HG_HANDLE_NULL) {

            hg_return_t ret = HG_SUCCESS;

            if(m_mercury_input) {
                ret = HG_Free_input(m_handle, m_mercury_input.get());

                HERMES_DEBUG2("HG_Free_input({}, {}) = {}", 
                              fmt::ptr(m_handle), fmt::ptr(&m_mercury_input), 
                              HG_Error_to_string(ret));
            }

            ret = HG_Destroy(m_handle);

            HERMES_DEBUG2("HG_Destroy({}) = {}", 
                          fmt::ptr(m_handle), HG_Error_to_string(ret));

            (void) ret; // avoid warnings if !DEBUG
        }
    }

private:
    hg_handle_t m_handle;
    std::unique_ptr<MercuryInput> m_mercury_input;
    std::unique_ptr<Input> m_input;
    bool m_requires_response;
};



} // namespace hermes

#endif // __HERMES_REQUEST_HPP__
