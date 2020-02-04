#ifndef __HERMES_RPCS_V2_HPP__
#define __HERMES_RPCS_V2_HPP__

// C includes
#include <mercury.h>
#include <mercury_proc_string.h>
#include <mercury_macros.h>

// C++ includes
#include <string>

// hermes includes
#include <hermes.hpp>

#ifndef HG_GEN_PROC_NAME
#define HG_GEN_PROC_NAME(struct_type_name) \
    hermes::detail::hg_proc_ ## struct_type_name
#endif

// forward declarations
namespace hermes { namespace detail {

template <typename ExecutionContext>
hg_return_t post_to_mercury(ExecutionContext* ctx);

}} // namespace hermes::detail

//==============================================================================
// definitions for example_rpcs::send_message
namespace hermes { namespace detail {

// Generate Mercury types and serialization functions (field names match
// those defined in send_message::input and send_message::output). These
// definitions are internal and should not be used directly. Classes
// send_message::input and send_message::output are provided for public use.
MERCURY_GEN_PROC(send_message_in_t,
        ((hg_const_string_t) (message)))

MERCURY_GEN_PROC(send_message_out_t,
        ((int32_t) (retval)))

}} // namespace hermes::detail

namespace example_rpcs {

struct send_message {

    // forward declarations of public input/output types for this RPC
    class input;
    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = send_message;
    using handle_type = hermes::rpc_handle<self_type>;
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

    // requires response?
    constexpr static const auto requires_response = true;

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


//==============================================================================
// definitions for example_rpcs::send_file
namespace hermes { namespace detail {

// Generate Mercury types and serialization functions (field names match
// those defined by send_file::input and send_file::output). These
// definitions are internal and should not be used directly. Classes
// send_file::input and send_file::output are provided for public use.
MERCURY_GEN_PROC(send_file_in_t,
        ((hg_const_string_t) (pathname))
        ((hg_bulk_t) (buffers)))

MERCURY_GEN_PROC(send_file_out_t,
        ((int32_t) (retval)))

}} // namespace hermes::detail

namespace example_rpcs {

struct send_file {

    // forward declarations of public input/output types for this RPC
    class input;
    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = send_file;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = hermes::detail::send_file_in_t;
    using mercury_output_type = hermes::detail::send_file_out_t;

    // RPC public identifier
    constexpr static const uint16_t public_id = 43;

    // RPC internal Mercury identifier
    constexpr static const uint16_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = "send_file";

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb = 
        HG_GEN_PROC_NAME(send_file_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb = 
        HG_GEN_PROC_NAME(send_file_out_t);

    class input {

        template <typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const std::string& pathname,
              const hermes::exposed_memory& buffers) :
            m_pathname(pathname),
            m_buffers(buffers) { }

        std::string
        pathname() const {
            return m_pathname;
        }

        hermes::exposed_memory
        buffers() const {
            return m_buffers;
        }

//TODO: make private
        explicit
        input(const hermes::detail::send_file_in_t& other) :
            m_pathname(other.pathname),
            m_buffers(other.buffers) { }
        
        explicit
        operator hermes::detail::send_file_in_t() {
            return {m_pathname.c_str(), hg_bulk_t(m_buffers)};
        }


    private:
        std::string m_pathname;
        hermes::exposed_memory m_buffers;
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
        output(const hermes::detail::send_file_out_t& out) {
            m_retval = out.retval;
        }

        explicit 
        operator hermes::detail::send_file_out_t() {
            return {m_retval};
        }

    private:
        int32_t m_retval;
    };
};

} // namespace example_rpcs


//==============================================================================
// definitions for example_rpcs::recv_buffer
namespace hermes { namespace detail {

// Generate Mercury types and serialization functions (field names match
// those defined by recv_buffer::input and recv_buffer::output). These
// definitions are internal and should not be used directly. Classes
// recv_buffer::input and recv_buffer::output are provided for public use.
MERCURY_GEN_PROC(recv_buffer_in_t,
        ((hg_const_string_t) (pathname))
        ((hg_bulk_t) (buffers)))

MERCURY_GEN_PROC(recv_buffer_out_t,
        ((int32_t) (retval)))

}} // namespace hermes::detail

namespace example_rpcs {

struct recv_buffer {

    // forward declarations of public input/output types for this RPC
    class input;
    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = recv_buffer;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = hermes::detail::recv_buffer_in_t;
    using mercury_output_type = hermes::detail::recv_buffer_out_t;

    // RPC public identifier
    constexpr static const uint16_t public_id = 44;

    // RPC internal Mercury identifier
    constexpr static const uint16_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = "recv_buffer";

    // requires response?
    constexpr static const auto requires_response = true;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb = 
        HG_GEN_PROC_NAME(recv_buffer_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb = 
        HG_GEN_PROC_NAME(recv_buffer_out_t);

    class input {

        template <typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input(const char* pathname,
              const hermes::exposed_memory& buffers) :
            m_pathname(pathname),
            m_buffers(buffers) { }

        input(const std::string& pathname,
              const hermes::exposed_memory& buffers) :
            m_pathname(pathname),
            m_buffers(buffers) { }

        std::string
        pathname() const {
            return m_pathname;
        }

        hermes::exposed_memory
        buffers() const {
            return m_buffers;
        }

//TODO: make private
        explicit
        input(const hermes::detail::recv_buffer_in_t& other) :
            m_pathname(other.pathname),
            m_buffers(other.buffers) { }
        
        explicit
        operator hermes::detail::recv_buffer_in_t() {
            return {m_pathname.c_str(), hg_bulk_t(m_buffers)};
        }

    private:
        std::string m_pathname;
        hermes::exposed_memory m_buffers;
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
        output(const hermes::detail::recv_buffer_out_t& out) {
            m_retval = out.retval;
        }

        explicit 
        operator hermes::detail::recv_buffer_out_t() {
            return {m_retval};
        }

    private:
        int32_t m_retval;
    };
};

} // namespace example_rpcs


//==============================================================================
// definitions for example_rpcs::shutdown
namespace hermes { namespace detail {

// Generate Mercury types and serialization functions (field names match
// those defined by shutdown::input and shutdown::output). These
// definitions are internal and should not be used directly. Classes
// shutdown::input and shutdown::output are provided for public use.

MERCURY_GEN_PROC(shutdown_in_t,
        ((int32_t) (foo))
);

MERCURY_GEN_PROC(shutdown_out_t,
        ((int32_t) (retval))
);

}} // namespace hermes::detail

namespace example_rpcs {

struct shutdown {

    // forward declarations of public input/output types for this RPC
    class input;
    class output;

    // traits used so that the engine knows what to do with the RPC
    using self_type = shutdown;
    using handle_type = hermes::rpc_handle<self_type>;
    using input_type = input;
    using output_type = output;
    using mercury_input_type = hermes::detail::shutdown_in_t;
    using mercury_output_type = hermes::detail::shutdown_out_t;

    // RPC public identifier
    constexpr static const uint16_t public_id = 45;

    // RPC internal Mercury identifier
    constexpr static const uint16_t mercury_id = public_id;

    // RPC name
    constexpr static const auto name = "shutdown";

    // requires response?
    constexpr static const auto requires_response = false;

    // Mercury callback to serialize input arguments
    constexpr static const auto mercury_in_proc_cb =
        HG_GEN_PROC_NAME(shutdown_in_t);

    // Mercury callback to serialize output arguments
    constexpr static const auto mercury_out_proc_cb = 
        HG_GEN_PROC_NAME(shutdown_out_t);

    class input {

        template <typename ExecutionContext>
        friend hg_return_t hermes::detail::post_to_mercury(ExecutionContext*);

    public:
        input() { }

//TODO: make private
        explicit
        input(const hermes::detail::shutdown_in_t& other) { 
            (void) other;
        }
        
        explicit
        operator hermes::detail::shutdown_in_t() {
            return {0};
        }
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
        output(const hermes::detail::shutdown_out_t& out) {
            m_retval = out.retval;
        }

        explicit 
        operator hermes::detail::shutdown_out_t() {
            return {m_retval};
        }

    private:
        int32_t m_retval;
    };
};

} // namespace example_rpcs

#undef HG_GEN_PROC_NAME

#endif // __HERMES_RPCS_V2_HPP__
