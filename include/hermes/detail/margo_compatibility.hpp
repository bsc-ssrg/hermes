#ifndef HERMES_DETAIL_MARGO_COMPATIBILTY_HPP
#define HERMES_DETAIL_MARGO_COMPATIBILTY_HPP

#ifndef __MARGO

#include <mercury.h>
#include <margo-hg-shim.h>
#include <pthread.h>

#define __MARGO_PROVIDER_ID_SIZE (sizeof(hg_id_t)/4)
#define __MARGO_RPC_HASH_SIZE (__MARGO_PROVIDER_ID_SIZE * 3)

#define MARGO_DEFAULT_PROVIDER_ID 0
#define MARGO_MAX_PROVIDER_ID ((1 << (8*__MARGO_PROVIDER_ID_SIZE))-1)

#endif // __MARGO

namespace hermes {
namespace detail {
namespace margo {

static inline void
demux_id(hg_id_t in,
         hg_id_t* base_id,
         uint16_t *provider_id) {
    /* retrieve low bits for provider */
    *provider_id = 0;
    *provider_id += (in & (((1<<(__MARGO_PROVIDER_ID_SIZE*8))-1)));

    /* clear low order bits */
    *base_id = (in >> (__MARGO_PROVIDER_ID_SIZE*8)) <<
        (__MARGO_PROVIDER_ID_SIZE*8);

    return;
}

static inline hg_id_t
mux_id(hg_id_t base_id,
       uint16_t provider_id) {
    hg_id_t id;

    id = (base_id >> (__MARGO_PROVIDER_ID_SIZE*8)) <<
       (__MARGO_PROVIDER_ID_SIZE*8);
    id |= provider_id;

    return id;
}

static pthread_key_t rpc_breadcrumb_key;
static pthread_once_t rpc_breadcrumb_key_once = PTHREAD_ONCE_INIT;

/* sets the value of a breadcrumb, to be called just before issuing an RPC */
static inline uint64_t
breadcrumb_set(hg_id_t rpc_id) {
    uint64_t* val;
    uint64_t tmp;

    (void) pthread_once(&rpc_breadcrumb_key_once,
            []() {
                (void) pthread_key_create(&rpc_breadcrumb_key, NULL);
            }
    );

    if((val = reinterpret_cast<uint64_t*>(
                    pthread_getspecific(rpc_breadcrumb_key))) == NULL) {
        // key not set yet on this thread; we need to allocate a new one
        // with all zeroes for initial value of breadcrumb and idx
        val = reinterpret_cast<uint64_t*>(calloc(1, sizeof(*val)));

        if(!val) {
            return 0;
        }
    }

    // NOTE: an rpc_id (after mux'ing) has provider in low order bits and
    // base rpc_id in high order bits.  After demuxing, a base_id has zeroed
    // out low bits.  So regardless of whether the rpc_id is a base_id or a
    // mux'd id, either way we need to shift right to get either the
    // provider id (or the space reserved for it) out of the way, then mask
    // off 16 bits for use as a breadcrumb.
    tmp = rpc_id >> (__MARGO_PROVIDER_ID_SIZE*8);
    tmp &= 0xffff;

    // clear low 16 bits of breadcrumb
    *val = (*val >> 16) << 16;

    // combine them, so that we have low order 16 of rpc id and high order
    // bits of previous breadcrumb
    *val |= tmp;

    (void) pthread_setspecific(rpc_breadcrumb_key, val);

    return *val;
}

static inline hg_id_t
HG_Register_name(hg_class_t* hg_class, const char* func_name,
                 hg_rpc_cb_t rpc_cb) {
    return HG_Register_name_for_margo(hg_class, func_name, rpc_cb);
}
static inline hg_return_t
HG_Forward(hg_handle_t handle, hg_cb_t cb, void* args, hg_proc_cb_t proc_in, void* in_struct) {
    return HG_Forward_to_margo(handle, cb, args, proc_in, in_struct);
}

static inline hg_return_t
HG_Respond(hg_handle_t handle, hg_cb_t cb, void* args, hg_proc_cb_t proc_out, void* out_struct) {
    return HG_Respond_to_margo(handle, cb, args, proc_out, out_struct);
}

static inline hg_return_t
HG_Get_input(hg_handle_t handle, hg_proc_cb_t proc_in, void* in_struct) {
    return HG_Get_input_from_margo(handle, proc_in, in_struct);
}

static inline hg_return_t
HG_Free_input(hg_handle_t handle, hg_proc_cb_t proc_in, void* in_struct) {
    return HG_Free_input_from_margo(handle, proc_in, in_struct);
}

static inline hg_return_t
HG_Get_output(hg_handle_t handle, hg_proc_cb_t proc_out, void* out_struct) {
    return HG_Get_output_from_margo(handle, proc_out, out_struct);
}

static inline hg_return_t
HG_Free_output(hg_handle_t handle, hg_proc_cb_t proc_out, void* out_struct) {
    return HG_Free_output_from_margo(handle, proc_out, out_struct);
}


} // namespace margo
} // namespace detail
} // namespace hermes

#endif // HERMES_DETAIL_MARGO_COMPATIBILTY_HPP
