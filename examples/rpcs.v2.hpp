#ifndef __HERMES_RPCS_V2_HPP__
#define __HERMES_RPCS_V2_HPP__

// C includes
#include <mercury_macros.h>
#include <mercury_proc_string.h>

// hermes includes
//#include <hermes/rpc.hpp>
//#include <hermes.hpp>
#include <hermes/request.hpp>

#include <hermes/detail/request_registrar.hpp>


// move to own header?
namespace hermes {
struct request_descriptor { };
} // namespace hermes;



//==============================================================================
// definitions for example_rpcs::send_message
namespace hermes { namespace detail {

// Generate Mercury types and serialization functions (field names match
// those defined by send_message_args and send_message_retval). These
// definitions are internal and should not be used directly. Classes
// send_message_args and send_message_retval are provided for public use.
MERCURY_GEN_PROC(send_message_in_t,
        ((hg_const_string_t) (message)))

MERCURY_GEN_PROC(send_message_out_t,
        ((int32_t) (retval)))

}} // namespace hermes::detail


#ifndef HG_GEN_PROC_NAME
#define HG_GEN_PROC_NAME(struct_type_name) \
    hermes::detail::hg_proc_ ## struct_type_name
#endif

namespace example_rpcs {

struct send_message {

    // forward declarations of public input/output types for this RPC
    class input;
    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = send_message;
    using handle_type = hermes::rpc_handle_v2<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = hermes::detail::send_message_in_t;
    using mercury_output_type = hermes::detail::send_message_out_t;

    // RPC public identifier
    constexpr static const uint16_t public_id = 42;

    // RPC internal Mercury identifier
    constexpr static const uint16_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = "send_message";

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb = 
        HG_GEN_PROC_NAME(send_message_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb = 
        HG_GEN_PROC_NAME(send_message_out_t);


    class input {

        template <typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& message) :
            m_message(message) { }

        std::string
        message() const {
            return m_message;
        }

        explicit
        input(const hermes::detail::send_message_in_t& other) :
            m_message(other.message) { }
        
        explicit
        operator hermes::detail::send_message_in_t() {
            return {m_message.c_str()};
        }


    private:
        std::string m_message;
    };

    class output {

        template <typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        output(int32_t retval) :
            m_retval(retval) { }

        int32_t
        retval() const {
            return m_retval;
        }

        void
        set_retval(int32_t retval) {
            m_retval = retval;
        }

        explicit 
        output(const hermes::detail::send_message_out_t& out) {
            m_retval = out.retval;
        }

        explicit 
        operator hermes::detail::send_message_out_t() {
            return {m_retval};
        }

    private:
        int32_t m_retval;
    };
};

} // namespace example_rpcs


namespace hermes { namespace detail {

static bool type1 __attribute__((used)) = 
    registered_requests().add<example_rpcs::send_message>();

}} // namespace hermes::detail

#undef HG_GEN_PROC_NAME

#endif // __HERMES_RPCS_V2_HPP__
