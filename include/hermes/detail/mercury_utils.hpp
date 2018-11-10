#ifndef __HERMES_DETAIL_MERCURY_UTILS_HPP__
#define __HERMES_DETAIL_MERCURY_UTILS_HPP__

#include <hermes/detail/request_status.hpp>

namespace hermes {
namespace detail {

inline hg_handle_t
create_handle(const hg_context_t* hg_context,
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
create_bulk_handle(hg_class_t* hg_class,
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

template <typename ExecutionContext, 
          typename MercuryInput, 
          typename MercuryOutput,
          typename RpcOutput>
void
forward(ExecutionContext* ctx,
        MercuryInput&& input) {

    DEBUG("Sending the RPC");

    const auto completion_callback = 
        [](const struct hg_cb_info* cbi) -> hg_return_t {

        auto* ctx = reinterpret_cast<ExecutionContext*>(cbi->arg);

        if(cbi->ret == HG_CANCELED) {
            switch(ctx->m_status) {
                case request_status::timeout: 
                {
                    DEBUG2("Request timed out, reposting");

                    auto&& hg_input = MercuryInput(ctx->m_input);

                    // repost the request
                    detail::forward<ExecutionContext, 
                                    MercuryInput, 
                                    MercuryOutput,
                                    RpcOutput>(
                                        ctx, 
                                        std::forward<MercuryInput>(hg_input));
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

        // decode response
        MercuryOutput output_val;
        hg_return_t ret = HG_Get_output(cbi->info.forward.handle, &output_val);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error(
                    std::string("Failed to decode RPC response: ") +
                                HG_Error_to_string(ret));
        }

        // TODO: check if this needs to be done in a critical region
        DEBUG2("Setting promise");

        ctx->m_output_promise.set_value(RpcOutput(output_val));

        // clean up resources consumed by this RPC

        HG_Free_output(cbi->info.forward.handle, &output_val);
        HG_Destroy(cbi->info.forward.handle);

    //    const struct hg_info* hgi = HG_Get_info(ctx->m_handle);

    //    HG_Addr_free(hgi->hg_class, hgi->addr);

        return HG_SUCCESS;
    };


    hg_return_t ret = HG_Forward(
            // Mercury handle
            ctx->m_handle,
            // pointer to function callback
            completion_callback,
            // pointer to data passed to callback
            reinterpret_cast<void*>(ctx),
            // pointer to input structure
            &input);

    DEBUG2("HG_Forward(handle={}, cb={}, arg={}, input={}) = {}",
            fmt::ptr(ctx->m_handle), 
            "lambda::completion_callback",
            fmt::ptr(ctx), 
            fmt::ptr(&input), ret);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error(
                std::string("Failed to send RPC: ") +
                HG_Error_to_string(ret));
    }



}



template <typename ExecutionContext, typename Callable>
inline void
bulk_transfer(request&& req,
              ExecutionContext&& ctx,
              Callable&& completion_callback) {

    const struct hg_info* hgi = HG_Get_info(req.m_handle);

    if(hgi == NULL) {
        throw std::runtime_error("Failed to retrieve request information "
                                 "from internal handle");
    }

    hg_size_t transfer_size = HG_Bulk_get_size(req.m_remote_bulk_handle);

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
            req.m_remote_bulk_handle,
            // origin offset
            0,
            // local bulk handle
            req.m_local_bulk_handle,
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
            fmt::ptr(hgi->addr), fmt::ptr(&req.m_remote_bulk_handle), 0,
            fmt::ptr(&req.m_local_bulk_handle), 0, transfer_size, ret);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to pull remote data: " +
                std::string(HG_Error_to_string(ret)));
    }
}

template <typename Output>
inline void
respond(request&& req, Output&& out) {

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

template <rpc ID, typename InputTp>
inline hg_return_t
rpc_handler(hg_handle_t handle) {
    using Descriptor = rpc_descriptor<ID>;
    using Executor = typename Descriptor::executor;


    return Executor::process(handle);
}

} // namespace detail
} // namespace hermes

#endif // __HERMES_DETAIL_MERCURY_UTILS_HPP__
