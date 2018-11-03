#ifndef __HERMES_RPC_DEFINITIONS_HPP__
#define __HERMES_RPC_DEFINITIONS_HPP__

// C++ includes
#include <unordered_map>
#include <functional>
#include <memory>
#include <hermes/messages.hpp>
#include <tuple>
#include <vector>

// C includes
#include <mercury_macros.h>
#include <mercury_proc_string.h>

// hermes includes
#include <hermes/event.hpp>
#include <hermes/bulk.hpp>

namespace hermes {

// forward declarations
class request;
class mutable_buffer;

/** Valid RPCs */
enum class rpc {
    __reserved__ = 0,  //XXX for some reason hg_id_t cannot be 0
    send_message,
    send_file,
    send_buffer,
    __end_count__
};

/** RPC names */
static const char* const rpc_names[] = {
    "__reserved__",
    "send_message",
    "send_file",
    "send_buffer"
};

/** enum class hash (required for upcoming std::unordered_maps */
struct enum_class_hash {
    template <typename T>
    std::size_t operator()(T t) const {
        return static_cast<std::size_t>(t);
    }
};


#ifndef HG_GEN_PROC_NAME
#define HG_GEN_PROC_NAME(struct_type_name) \
    hg_proc_ ## struct_type_name
#endif


/** Generate types and serialization functions for rpc::send_message */
MERCURY_GEN_PROC(send_message_in_t,
        ((hg_const_string_t) (message)))

MERCURY_GEN_PROC(send_message_out_t,
        ((int32_t) (ret_val)))


/** Generate types and serialization functions for rpc::send_file */
MERCURY_GEN_PROC(send_file_in_t,
        ((hg_const_string_t) (pathname))
        ((hg_bulk_t) (bulk_handle)))

MERCURY_GEN_PROC(send_file_out_t,
        ((int32_t) (ret_val)))


/** Generate types and serialization functions for rpc::send_file */
MERCURY_GEN_PROC(send_buffer_in_t,
        ((hg_const_string_t) (pathname))
        ((hg_bulk_t) (bulk_handle)))

MERCURY_GEN_PROC(send_buffer_out_t,
        ((int32_t) (ret_val)))


namespace detail { 
template <rpc ID> struct origin_context; 
template <rpc ID, typename MessageTp> struct rpc_processor; 
}


class send_message_args {

    friend class detail::origin_context<rpc::send_message>;
    friend class detail::rpc_processor<rpc::send_message, message::simple>;

public:
    send_message_args(const std::string& message) :
        m_message(message) { }

    std::string
    get_message() const {
        return m_message;
    }

private:
    explicit send_message_args(send_message_in_t input) :
        m_message(input.message) { }

    explicit operator send_message_in_t() {
        return {m_message.c_str()};
    }

    std::string m_message;
};

class send_message_retval { };


class send_file_args {

    friend class detail::origin_context<rpc::send_file>;
    friend class detail::rpc_processor<rpc::send_file, message::bulk>;

public:
    send_file_args(const std::string& pathname,
                   const exposed_memory& memory) :
        m_pathname(pathname),
        m_exposed_memory(memory) { }

    std::string
    get_pathname() const {
        return m_pathname;
    }

    exposed_memory
    get_exposed_memory() const {
        return m_exposed_memory;
    }

private:
    explicit send_file_args(send_file_in_t input) :
        m_pathname(input.pathname),
        m_exposed_memory(input.bulk_handle) { }

    explicit operator send_file_in_t() {
        return {m_pathname.c_str(), hg_bulk_t(m_exposed_memory)};
    }

    std::string m_pathname;
    exposed_memory m_exposed_memory;
};

class send_file_retval { };


class send_buffer_args {

    friend class detail::origin_context<rpc::send_buffer>;
    friend class detail::rpc_processor<rpc::send_buffer, message::bulk>;

public:
    send_buffer_args(const std::string& pathname,
                     const exposed_memory& memory) :
        m_pathname(pathname),
        m_exposed_memory(memory) { }

    std::string
    get_pathname() const {
        return m_pathname;
    }

    exposed_memory
    get_exposed_memory() const {
        return m_exposed_memory;
    }

private:
    explicit send_buffer_args(send_buffer_in_t input) :
        m_pathname(input.pathname),
        m_exposed_memory(input.bulk_handle) { }

    explicit operator send_buffer_in_t() {
        return {m_pathname.c_str(), hg_bulk_t(m_exposed_memory)};
    }

    std::string m_pathname;
    exposed_memory m_exposed_memory;
};

class send_buffer_retval { };



namespace detail {

// TODO: rename to rpc_info_base
struct rpc_descriptor_base {

protected:
    rpc_descriptor_base(const hermes::rpc id,
                        const char* const name,
                        const hg_id_t hg_id,
                        const hg_proc_cb_t in_proc_cb,
                        const hg_proc_cb_t out_proc_cb,
                        const hg_rpc_cb_t rpc_cb) :
        m_id(id),
        m_name(name),
        m_hg_id(hg_id),
        m_in_proc_cb(in_proc_cb),
        m_out_proc_cb(out_proc_cb),
        m_rpc_cb(rpc_cb) {}

protected:
    rpc_descriptor_base(const rpc_descriptor_base&) = delete;
    rpc_descriptor_base(rpc_descriptor_base&&) = default;
    rpc_descriptor_base& operator=(const rpc_descriptor_base&) = delete;
    rpc_descriptor_base& operator=(rpc_descriptor_base&&) = default;
    ~rpc_descriptor_base() = default;

public:
    const hermes::rpc m_id;
    const char* const m_name;
    const hg_id_t m_hg_id;
    const hg_proc_cb_t m_in_proc_cb;
    const hg_proc_cb_t m_out_proc_cb;
    const hg_rpc_cb_t m_rpc_cb;
};

// forward declarations
template <rpc ID, typename InputTp>
hg_return_t
rpc_handler(hg_handle_t handle);


template <rpc>
struct rpc_descriptor : public rpc_descriptor_base {};

template <rpc ID, typename MsgTp>
struct rpc_processor;

/******************************************************************************/
/** specialized rpc_info for rpc::message */
template <>
struct rpc_descriptor<rpc::send_message> : public rpc_descriptor_base {

    using input_type = send_message_args;
    using output_type = send_message_retval;
    using mercury_input_type = send_message_in_t;
    using mercury_output_type = send_message_out_t;

    // convenience typedef
    using utype = std::underlying_type<event>::type;

    using input_arg_type = send_message_in_t;
    using output_arg_type = send_message_out_t;
    using message_type = message::simple;
    using processor = rpc_processor<rpc::send_message, message_type>;
    using requires_reply = std::true_type;

    rpc_descriptor() :
        rpc_descriptor_base(
            rpc::send_message,
            rpc_names[static_cast<int>(rpc::send_message)],
            static_cast<hg_id_t>(rpc::send_message),
            //hg_hash_string(rpc_names[0]),
            HG_GEN_PROC_NAME(send_message_in_t),
            HG_GEN_PROC_NAME(send_message_out_t),
            rpc_handler<rpc::send_message, send_message_in_t>) {}

    template <typename Callable>
    void
    set_handler(const event trigger, 
                Callable&& handler) {

        switch(trigger) {
            case event::on_arrival:
            case event::on_completion:
                m_handlers[static_cast<utype>(trigger)] = 
                    std::forward<Callable>(handler);
                break;
            default:
                throw std::runtime_error("Unexpected event");
        }
    }

    output_type
    invoke_handler(const event trigger,
                   const request& req,
                   const input_type& args) {

        switch(trigger) {
            case event::on_arrival:
            case event::on_completion:
                if(m_handlers[static_cast<utype>(trigger)]) {
                    return m_handlers[static_cast<utype>(trigger)](req, args);
                }
                break;
            default:
                throw std::runtime_error("Unexpected event");
        }

        return output_type();
    }

    using handler_type =
        std::function<output_type(const request&, const input_type&)>;
    std::array<handler_type, static_cast<utype>(event::count)> m_handlers;
};


/******************************************************************************/
/** specialized rpc_info for rpc::send_file */
template <>
struct rpc_descriptor<rpc::send_file> : public rpc_descriptor_base {

    using input_type = send_file_args;
    using output_type = send_file_retval;
    using mercury_input_type = send_file_in_t;
    using mercury_output_type = send_file_out_t;

    // convenience typedef
    using utype = std::underlying_type<event>::type;

    using input_arg_type = send_file_in_t;
    using output_arg_type = send_file_out_t;
    using message_type = message::bulk;
    using processor = rpc_processor<rpc::send_file, message_type>;
    using requires_reply = std::true_type;

    rpc_descriptor() :
        rpc_descriptor_base(
            rpc::send_file,
            rpc_names[static_cast<int>(rpc::send_file)],
            static_cast<hg_id_t>(rpc::send_file),
            //hg_hash_string(rpc_names[0]),
            HG_GEN_PROC_NAME(send_file_in_t),
            HG_GEN_PROC_NAME(send_file_out_t),
            rpc_handler<rpc::send_file, send_file_in_t>) {}

    template <typename Callable>
    void
    set_handler(const event trigger, 
                Callable&& handler) {

        switch(trigger) {
            case event::on_arrival:
            case event::on_completion:
                m_handlers[static_cast<utype>(trigger)] = 
                    std::forward<Callable>(handler);
                break;
            default:
                throw std::runtime_error("Unexpected event");
        }
    }

    output_type
    invoke_handler(const event trigger, 
                   const request& req,
                   const input_type& args) {

        switch(trigger) {
            case event::on_arrival:
            case event::on_completion:
                if(m_handlers[static_cast<utype>(trigger)]) {
                    return m_handlers[static_cast<utype>(trigger)](req, args);
                }
                break;
            default:
                throw std::runtime_error("Unexpected event");
        }

        return output_type();
    }

    using handler_type =
        std::function<output_type(const request&, const input_type&)>;
    std::array<handler_type, static_cast<utype>(event::count)> m_handlers;
};


/******************************************************************************/
/** specialized rpc_info for rpc::send_buffer */
template <>
struct rpc_descriptor<rpc::send_buffer> : public rpc_descriptor_base {

    using input_type = send_buffer_args;
    using output_type = send_buffer_retval;
    using mercury_input_type = send_buffer_in_t;
    using mercury_output_type = send_buffer_out_t;

    // convenience typedef
    using utype = std::underlying_type<event>::type;

    using input_arg_type = send_buffer_in_t;
    using output_arg_type = send_buffer_out_t;
    using message_type = message::bulk;
    using processor = rpc_processor<rpc::send_buffer, message_type>;
    using requires_reply = std::true_type;

    rpc_descriptor() :
        rpc_descriptor_base(
            rpc::send_buffer,
            rpc_names[static_cast<int>(rpc::send_buffer)],
            static_cast<hg_id_t>(rpc::send_buffer),
            //hg_hash_string(rpc_names[0]),
            HG_GEN_PROC_NAME(send_buffer_in_t),
            HG_GEN_PROC_NAME(send_buffer_out_t),
            rpc_handler<rpc::send_buffer, send_buffer_in_t>) {}

    template <typename Callable>
    void
    set_handler(const event trigger, 
                Callable&& handler) {

        switch(trigger) {
            case event::on_arrival:
            case event::on_completion:
                m_handlers[static_cast<utype>(trigger)] = 
                    std::forward<Callable>(handler);
                break;
            default:
                throw std::runtime_error("Unexpected event");
        }
    }

    output_type
    invoke_handler(const event trigger, 
                   const request& req,
                   const input_type& args) {

        switch(trigger) {
            case event::on_arrival:
            case event::on_completion:
                if(m_handlers[static_cast<utype>(trigger)]) {
                    return m_handlers[static_cast<utype>(trigger)](req, args);
                }
                break;
            default:
                throw std::runtime_error("Unexpected event");
        }

        return output_type();
    }

    using handler_type = 
        std::function<output_type(const request&, const input_type&)>;
    std::array<handler_type, static_cast<utype>(event::count)> m_handlers;
};

static const
std::unordered_map<
    rpc,
    std::shared_ptr<detail::rpc_descriptor_base>,
    enum_class_hash
> registered_rpcs_v2 = {
    {
        rpc::send_message,
        std::make_shared<rpc_descriptor<rpc::send_message>>()
    },

    {
        rpc::send_file,
        std::make_shared<rpc_descriptor<rpc::send_file>>()
    },

    {
        rpc::send_buffer,
        std::make_shared<rpc_descriptor<rpc::send_buffer>>()
    },
};

} // namespace detail

//TODO: remove in favor of local aliases
//template <rpc ID>
//using RpcInputArgTp = 
//    typename detail::rpc_descriptor<ID>::input_arg_type;
//
//
//template <rpc ID>
//using RpcOutputArgTp = 
//    typename detail::rpc_descriptor<ID>::output_arg_type;



template <rpc ID>
using RpcInput = 
    typename detail::rpc_descriptor<ID>::input_type;

template <rpc ID>
using RpcOutput = 
    typename detail::rpc_descriptor<ID>::output_type;

template <rpc ID>
using MercuryInput = 
    typename detail::rpc_descriptor<ID>::mercury_input_type;

template <rpc ID>
using MercuryOutput = 
    typename detail::rpc_descriptor<ID>::mercury_output_type;

#ifdef HG_GEN_PROC_NAME
#undef HG_GEN_PROC_NAME
#endif







using maker_function = std::function<
    std::shared_ptr<detail::rpc_descriptor_base>()>;

std::array<
    maker_function,
    static_cast<unsigned int>(rpc::__end_count__)
> makers2 {
     nullptr,
     std::make_shared<detail::rpc_descriptor<rpc::send_message>>, 
     std::make_shared<detail::rpc_descriptor<rpc::send_file>>, 
     std::make_shared<detail::rpc_descriptor<rpc::send_buffer>>, 
};



} // namespace hermes

#endif // __HERMES_RPC_DEFINITIONS_HPP__
