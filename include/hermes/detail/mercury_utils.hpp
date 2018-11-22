#ifndef __HERMES_DETAIL_MERCURY_UTILS_HPP__
#define __HERMES_DETAIL_MERCURY_UTILS_HPP__

// C includes
#include <mercury.h>

// project includes
#include <hermes/detail/request_registrar.hpp>
#include <hermes/detail/request_status.hpp>

namespace hermes {

// defined elsewhere
template <typename Request>
class request;

namespace detail {

template <typename Descriptor>
inline void
register_mercury_rpc(hg_class_t* hg_class, 
                     const Descriptor& descriptor,
                     bool listen) {

    hg_return_t ret = HG_Register(hg_class,
                                  descriptor->m_mercury_id,
                                  descriptor->m_mercury_input_cb,
                                  descriptor->m_mercury_output_cb,
                                  listen ? 
                                    descriptor->m_handler : 
                                    nullptr);

    DEBUG2("    HG_Register(hg_class={}, hg_id={}, in_proc_cb={}, "
                            "out_proc_cb={}, rpc_cb={}) = {}",
            static_cast<void*>(hg_class), 
            descriptor->m_mercury_id,
            reinterpret_cast<void*>(descriptor->m_mercury_input_cb),
            reinterpret_cast<void*>(descriptor->m_mercury_output_cb),
            (listen ? (void*)(descriptor->m_handler) : nullptr),
            ret);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to register RPC '" +
                            std::string(descriptor->m_name) + "': " +
                            std::string(HG_Error_to_string(ret)));
    }
}

inline hg_handle_t
create_mercury_handle(const hg_context_t* hg_context,
                      const hg_addr_t hg_addr,
                      const hg_id_t hg_id) {

    DEBUG("Creating Mercury handle");

    hg_handle_t handle = HG_HANDLE_NULL;

    hg_return_t ret = HG_Create(
                // pointer to the HG context
                const_cast<hg_context_t*>(hg_context),
                // address of destination
                const_cast<hg_addr_t>(hg_addr),
                // registered RPC ID
                hg_id,
                // pointer to HG handle
                &handle);

    DEBUG2("HG_Create(hg_context={}, addr={}, hg_id={}, hg_handle={}) = {}",
            fmt::ptr(hg_context), fmt::ptr(&hg_addr),
            hg_id, fmt::ptr(&handle), ret);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error(
                std::string("Failed to create handle: ") +
                HG_Error_to_string(ret));
    }

    return handle;
}

inline hg_bulk_t
create_mercury_bulk_handle(hg_class_t* hg_class,
                           const hg_uint32_t buf_count,
                           void** buf_ptrs,
                           const hg_size_t* buf_sizes,
                           const hg_uint8_t flags) {

    DEBUG("Creating Mercury bulk handle");

    hg_bulk_t bulk_handle = HG_BULK_NULL;

    hg_return_t ret = HG_Bulk_create(
                    // pointer to Mercury class
                    hg_class, 
                    // number of buffers
                    buf_count,
                    // array of pointers to buffers 
                    buf_ptrs,
                    // array of buffer sizes
                    buf_sizes,
                    // flags (e.g. HG_BULK_READ_ONLY, HG_BULK_WRITE_ONLY)
                    flags,
                    // pointer to returned bulk handle
                    &bulk_handle);

    DEBUG2("HG_Bulk_create(hg_class={}, count={}, buf_ptrs={}, buf_sizes={}, "
           "flags={}, handle={}) = {}", fmt::ptr(hg_class), buf_count,
           fmt::ptr(buf_ptrs), fmt::ptr(buf_sizes), flags,
           fmt::ptr(&bulk_handle), ret);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to create bulk handle: " + 
                std::string(HG_Error_to_string(ret)));
    }

    return bulk_handle;
}

template <typename Request>
static inline typename Request::mercury_input_type
decode_mercury_input(hg_handle_t handle) {

    //using Input = typename Request::input_type;
    using MercuryInput = typename Request::mercury_input_type;

    if(handle == HG_HANDLE_NULL) {
        throw std::runtime_error("Invalid handle passed to "
                                 "decode_mercury_input()");
    }

    MercuryInput hg_input;

    // decode input
    hg_return_t ret = HG_Get_input(handle, &hg_input);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to decode request input data: " +
                std::string(HG_Error_to_string(ret)));
    }

    return hg_input;
}

template <typename Request>
static inline typename Request::mercury_output_type
decode_mercury_output(hg_handle_t handle) {

    //using Output = typename Request::output_type;
    using MercuryOutput = typename Request::mercury_output_type;

    if(handle == HG_HANDLE_NULL) {
        throw std::runtime_error("Invalid handle passed to "
                                 "decode_mercury_input()");
    }

    MercuryOutput hg_output;

    // decode output
    hg_return_t ret = HG_Get_output(handle, &hg_output);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to decode request output data: " +
                std::string(HG_Error_to_string(ret)));
    }

    return hg_output;

#if 0
    Output rv = Output(hg_output);

    // It is now safe to delete the output provided by Mercury, since
    // we now have a copy of the user's return value
    HG_Free_output(handle, &hg_output);

    return rv;
#endif
}

template <typename ExecutionContext>
hg_return_t
post_to_mercury(ExecutionContext* ctx) {

    using Request = typename ExecutionContext::value_type;
    // using MercuryInput = typename Request::mercury_input_type;
    using MercuryOutput = typename Request::mercury_output_type;
    // using RequestInput = typename Request::input_type;
    using RequestOutput = typename Request::output_type;

    DEBUG("Sending request");

    const auto completion_callback = 
        [](const struct hg_cb_info* cbi) -> hg_return_t {

        auto* ctx = reinterpret_cast<ExecutionContext*>(cbi->arg);

        if(cbi->ret == HG_CANCELED) {
            switch(ctx->m_status) {
                case request_status::timeout: 
                {
                    DEBUG2("Request timed out, reposting");
                    // repost the request
                    post_to_mercury<ExecutionContext>(ctx); 
                    break;
                }

                case request_status::cancelled:
                {
                    // this can only happen if the request timed out repeteadly
                    // and we exhausted the configured retries
                    // The only option is to definitely cancel the request and
                    // set an exception for the user
                    DEBUG2("Request was cancelled");

                    ctx->m_output_promise.set_exception(
                            std::make_exception_ptr(
                                std::runtime_error("Request timed out")));

                    if(cbi->info.forward.handle != HG_HANDLE_NULL) {
                        HG_Destroy(cbi->info.forward.handle);
                    }
                    break;
                }

                default:
                    DEBUG2("Request is in an inconsistent state");
                    ctx->m_output_promise.set_exception(
                            std::make_exception_ptr(
                                std::runtime_error("Request is in an "
                                                   "inconsistent state")));

                    if(cbi->info.forward.handle != HG_HANDLE_NULL) {
                        HG_Destroy(cbi->info.forward.handle);
                    }
            }

            return cbi->ret;
        }

        if(cbi->ret != HG_SUCCESS) {
            DEBUG("Forward request failed: {}", HG_Error_to_string(cbi->ret));

            ctx->m_output_promise.set_exception(
                    std::make_exception_ptr(
                        std::runtime_error("Request failed: " + 
                            std::string(HG_Error_to_string(cbi->ret)))));

            if(cbi->info.forward.handle != HG_HANDLE_NULL) {
                HG_Destroy(cbi->info.forward.handle);
            }

            return cbi->ret;
        }

        DEBUG2("Setting output promise");

        MercuryOutput hg_output = 
            detail::decode_mercury_output<Request>(
                    cbi->info.forward.handle);

        ctx->m_output_promise.set_value(RequestOutput(hg_output));
                
        // clean up resources consumed by this RPC
        HG_Free_output(cbi->info.forward.handle, &hg_output);
        HG_Destroy(cbi->info.forward.handle);

        return HG_SUCCESS;
    };


    // if the ExecutionContext already has a handle, we are probably reposting
    // the request. In this case, we don't need to create a new handle, we can
    // just reuse the old one since the Mercury documentation states that 
    // this is safe to do
    if(ctx->m_handle == HG_HANDLE_NULL) {
        // create a Mercury handle for the RPC and save it in the RPC's 
        // execution context
        ctx->m_handle = detail::create_mercury_handle(ctx->m_hg_context,
                                                      ctx->m_address->fetch(),
                                                      Request::mercury_id);
    }

    hg_return_t ret = HG_Forward(
            // Mercury handle
            ctx->m_handle,
            // pointer to function callback
            completion_callback,
            // pointer to data passed to callback
            reinterpret_cast<void*>(ctx),
            // pointer to input structure
            &ctx->m_mercury_input);

    DEBUG2("HG_Forward(handle={}, cb={}, arg={}, input={}) = {}",
            fmt::ptr(ctx->m_handle), 
            "lambda::completion_callback",
            fmt::ptr(ctx), 
            fmt::ptr(&ctx->m_mercury_input), 
            ret);

    return ret;
}


template <typename ExecutionContext>
inline void
mercury_bulk_transfer(hg_handle_t handle, 
                      hg_bulk_t remote_bulk_handle,
                      hg_bulk_t local_bulk_handle,
                      ExecutionContext* ctx,
                      hg_cb_t completion_callback) {

    const struct hg_info* hgi = HG_Get_info(handle);

    if(!hgi) {
        throw std::runtime_error("Failed to retrieve request information "
                                 "from internal handle");
    }

    hg_size_t transfer_size = HG_Bulk_get_size(remote_bulk_handle);

    if(transfer_size == 0) {
        throw std::runtime_error("Bulk size to transfer is 0");
    }

    hg_return_t ret = HG_Bulk_transfer(
            // pointer to Mercury context
            hgi->context,
            // pointer to function callback
            completion_callback,
            // pointer to data passed to callback
            reinterpret_cast<void*>(ctx),
            // transfer operation: pull from client
            HG_BULK_PULL,
            // address of origin
            hgi->addr,
            // bulk handle from origin
            remote_bulk_handle,
            // origin offset
            0,
            // local bulk handle
            local_bulk_handle,
            // local offset
            0,
            // size of data to be transferred
            transfer_size,
            // pointer to returned operation ID
            HG_OP_ID_IGNORE);

    DEBUG2("HG_Bulk_transfer(hg_context={}, callback={}, arg={}, op={}, "
            "addr={}, origin_handle={}, origin_offset={}, local_handle={}, "
            "local_offset={}, size={}, HG_OP_ID_IGNORE) = {}", 
            fmt::ptr(hgi->context),
            "lambda::completion_callback", fmt::ptr(ctx), "HG_BULK_PULL", 
            fmt::ptr(hgi->addr), fmt::ptr(&remote_bulk_handle), 0,
            fmt::ptr(&local_bulk_handle), 0, transfer_size, ret);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to pull remote data: " +
                std::string(HG_Error_to_string(ret)));
    }
}

template <typename Input, typename Output>
inline void
mercury_respond(request<Input>&& req, 
                Output&& out) {

    // This is just a best effort response, we don't bother specifying
    // a callback here for completion
    hg_return_t ret = HG_Respond(
                            // handle
                            req.m_handle,
                            // callback
                            NULL,
                            // arg
                            NULL,
                            // output struct
                            &out);

    DEBUG2("HG_Respond(hg_handle={}, callback={}, arg={}, out_struct={}) = {}", 
           fmt::ptr(req.m_handle), "NULL", "NULL", fmt::ptr(&out), ret);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to respond: " + 
                std::string(HG_Error_to_string(ret)));
    }
}


template <typename Request>
static inline hg_return_t
mercury_handler(hg_handle_t handle) {

    // using input_type = typename Request::input_type;
    // using mercury_input_type = typename Request::mercury_input_type;

    const auto descriptor = 
        std::static_pointer_cast<detail::request_descriptor<Request>>(
            detail::registered_requests().at(Request::public_id));

    if(!descriptor) {
        throw std::runtime_error("Requested descriptor for request "
                                 "of unknown type");
    }

    descriptor->invoke_user_handler(std::move(request<Request>(handle)));

    return HG_SUCCESS;
}


} // namespace detail
} // namespace hermes

#endif // __HERMES_DETAIL_MERCURY_UTILS_HPP__
