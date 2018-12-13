#ifndef __HERMES_REQUEST_DESCRIPTOR_HPP__
#define __HERMES_REQUEST_DESCRIPTOR_HPP__

// C includes
#include <mercury.h>

// C++ includes
#include <functional>

// project includes
#include <hermes/logging.hpp>

namespace hermes {

// defined in this file
class request_descriptor_base;

template <typename Request>
struct request_descriptor;

// defined elsewhere
template <typename Request>
class request;

// defined elsewhere
class async_engine;

namespace detail {

// defined elsewhere
template <typename Request>
static inline hg_return_t
mercury_handler(hg_handle_t handle);

class request_descriptor_base {

public:
    request_descriptor_base(uint16_t id,
                            const hg_id_t hg_id,
                            const char* const name,
                            const bool requires_response,
                            const hg_proc_cb_t in_proc_cb,
                            const hg_proc_cb_t out_proc_cb,
                            const hg_rpc_cb_t handler) :
        m_id(id),
        m_mercury_id(hg_id),
        m_name(name),
        m_requires_response(requires_response),
        m_mercury_input_cb(in_proc_cb),
        m_mercury_output_cb(out_proc_cb),
        m_handler(handler) {}

    virtual ~request_descriptor_base() = default;

protected:
    request_descriptor_base(request_descriptor_base&&) = default;
    request_descriptor_base(const request_descriptor_base&) = delete;
    request_descriptor_base& operator=(request_descriptor_base&&) = default;
    request_descriptor_base& operator=(const request_descriptor_base&) = delete;

public:
    const uint16_t m_id;
    const hg_id_t m_mercury_id;
    const char* const m_name;
    const bool m_requires_response;
    const hg_proc_cb_t m_mercury_input_cb;
    const hg_proc_cb_t m_mercury_output_cb;
    const hg_rpc_cb_t m_handler;
};

template <typename Request>
struct request_descriptor : public request_descriptor_base {

    // traits used so that the engine knows what to do with the RPC
    using request_type = Request;
    using handle_type = typename Request::handle_type;
    using input_type = typename Request::input_type;
    using output_type = typename Request::output_type;
    using mercury_input_type = typename Request::mercury_input_type;
    using mercury_output_type = typename Request::mercury_output_type;

    // RPC public identifier
    constexpr static const auto public_id = Request::public_id;

    // RPC internal Mercury identifier
    constexpr static const auto mercury_id = public_id;

    // RPC name
    constexpr static const auto name = Request::name;

    // requires response?
    constexpr static const auto requires_response = Request::requires_response;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb = 
        Request::mercury_in_proc_cb;

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb = 
        Request::mercury_out_proc_cb;


    request_descriptor(uint16_t id,
                       const hg_id_t hg_id,
                       const char* const name,
                       const bool requires_response,
                       const hg_proc_cb_t in_proc_cb,
                       hg_proc_cb_t out_proc_cb) :
        request_descriptor_base(id, 
                                hg_id, 
                                name, 
                                requires_response,
                                in_proc_cb, 
                                out_proc_cb, 
                                mercury_handler<request_type>) {}

    template <typename Callable>
    void
    set_user_handler(Callable&& handler) {

        HERMES_DEBUG2("Setting user handler for requests of type \"{}\"", 
                      std::string(name));
        m_user_handler = std::forward<Callable>(handler);
    }

    template <typename RequestHandle>
    void
    invoke_user_handler(RequestHandle&& req) {

        if(!m_user_handler) {
            throw std::runtime_error("User handler for request [" + 
                    std::string(name) +  "] not set");
        }

        m_user_handler(std::forward<RequestHandle>(req));
    }

private:
    using handler_type = std::function<void(hermes::request<request_type>&&)>;
    handler_type m_user_handler;
};

} // namespace detail
} // namespace hermes

#endif // __HERMES_REQUEST_DESCRIPTOR_HPP__
