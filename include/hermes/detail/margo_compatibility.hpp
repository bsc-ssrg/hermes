#ifndef HERMES_DETAIL_MARGO_COMPATIBILTY_HPP
#define HERMES_DETAIL_MARGO_COMPATIBILTY_HPP

#ifndef __MARGO

#include <mercury.h>
#include <margo-hg-shim.h>
#include <pthread.h>

#endif // __MARGO

namespace hermes {
namespace detail {
namespace margo {

static inline hg_id_t
register_name(hg_class_t* hg_class, const char* func_name,
                 hg_rpc_cb_t rpc_cb) {
    return HG_Register_name_for_margo(hg_class, func_name, rpc_cb);
}
static inline hg_return_t
forward(hg_handle_t handle, hg_cb_t cb, void* args, hg_proc_cb_t proc_in, void* in_struct) {
    return HG_Forward_to_margo(handle, cb, args, proc_in, in_struct);
}

static inline hg_return_t
respond(hg_handle_t handle, hg_cb_t cb, void* args, hg_proc_cb_t proc_out, void* out_struct) {
    return HG_Respond_to_margo(handle, cb, args, proc_out, out_struct);
}

static inline hg_return_t
get_input(hg_handle_t handle, hg_proc_cb_t proc_in, void* in_struct) {
    return HG_Get_input_from_margo(handle, proc_in, in_struct);
}

static inline hg_return_t
free_input(hg_handle_t handle, hg_proc_cb_t proc_in, void* in_struct) {
    return HG_Free_input_from_margo(handle, proc_in, in_struct);
}

static inline hg_return_t
get_output(hg_handle_t handle, hg_proc_cb_t proc_out, void* out_struct) {
    return HG_Get_output_from_margo(handle, proc_out, out_struct);
}

static inline hg_return_t
free_output(hg_handle_t handle, hg_proc_cb_t proc_out, void* out_struct) {
    return HG_Free_output_from_margo(handle, proc_out, out_struct);
}

} // namespace margo
} // namespace detail
} // namespace hermes

#endif // HERMES_DETAIL_MARGO_COMPATIBILTY_HPP
