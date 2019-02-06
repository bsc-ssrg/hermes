#ifndef __HERMES_DETAIL_EXECUTION_CONTEXT_HPP__
#define __HERMES_DETAIL_EXECUTION_CONTEXT_HPP__

// C includes
#include <mercury.h>

// C++ includes
#include <memory>
#include <atomic>
#include <future>

// project includes
#include <hermes/detail/request_status.hpp>

namespace hermes {

// defined elsewhere
template <typename Request>
class rpc_handle;

namespace detail {

struct address;

/** Execution context required by the RPC's originator (i.e. the client) */
template <typename Request>
struct execution_context {

    using value_type = Request;
    using Handle = rpc_handle<Request>;
    using Input = typename Request::input_type;
    using Output = typename Request::output_type;
    using MercuryInput = typename Request::mercury_input_type;
    using MercuryOutput = typename Request::mercury_output_type;

    execution_context(Handle* parent,
                      const hg_context_t * const hg_context,
                      const std::shared_ptr<detail::address>& address,
                      Input&& input) :
        m_parent(parent),
        m_hg_context(hg_context),
        m_handle(HG_HANDLE_NULL),
        m_bulk_handle(HG_BULK_NULL),
        m_status(detail::request_status::created),
        m_address(address),
        // the context takes ownership of the user's input in order to ensure
        // that any pointers in m_mercury_input that refer back to it (e.g. 
        // strings) survives as long as needed (this is required because
        // mercury does not take ownership of any pointers in the user input)
        m_user_input(std::move(input)),
        // invoke explicit cast constructor
        m_mercury_input(m_user_input) { }

    Handle* m_parent;
    const hg_context_t* const m_hg_context;
    hg_handle_t m_handle;
    hg_bulk_t m_bulk_handle;
    std::atomic<detail::request_status> m_status;

    const std::shared_ptr<detail::address> m_address;
    Input m_user_input;
    MercuryInput m_mercury_input;

    std::promise<Output> m_output_promise;
};

} // namespace detail
} // namespace hermes

#endif // __HERMES_DETAIL_EXECUTION_CONTEXT_HPP__
