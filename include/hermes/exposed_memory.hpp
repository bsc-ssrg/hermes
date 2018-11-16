#ifndef __HERMES_BULK_HPP__
#define __HERMES_BULK_HPP__


// C includes
#include <mercury.h>
#include <mercury_bulk.h>
#include <alloca.h>

// C++ includes
#include <cassert>

// project includes
#include <hermes/buffer.hpp>
#include <hermes/logging.hpp>


namespace hermes {

enum class access_mode : hg_uint32_t {
    read_only  = HG_BULK_READ_ONLY,
    write_only = HG_BULK_WRITE_ONLY,
    read_write = HG_BULK_READWRITE
};

/**
 * The @c exposed_memory class represent abstractions of memory segments exposed
 * by a process for RDMA operations. A exposed_memory object can explicitly be
 * converted to a hg_bulk_t so that Mercury can send it over RPC to another
 * process.  */
class exposed_memory {

    friend class async_engine;

public:

    using iterator = std::vector<mutable_buffer>::iterator;

    /** Constructs an empty exposed_memory object */
    exposed_memory() { }

    /** Constructs a @c exposed_memory object from a @c BufferSequence */
    template <typename BufferSequence>
    exposed_memory(const hg_class_t* hg_class,
                   access_mode mode,
                   BufferSequence&& bufseq) :
        m_hg_class(hg_class),
        m_mode(mode),
        m_size(0) {

        m_buffers.reserve(bufseq.size());
        for(auto&& buf : bufseq) {
            m_size += buf.size();
            m_buffers.emplace_back(buf);
        }
    }

    /** Converts between a Mercury bulk handle and a exposed_memory object */
    explicit exposed_memory(hg_bulk_t bulk_handle) { 

        DEBUG("Converting between hg_bulk_t and hermes::exposed_memory");

        hg_uint32_t bulk_count = HG_Bulk_get_segment_count(bulk_handle);
        hg_size_t bulk_size = HG_Bulk_get_size(bulk_handle);

        if(bulk_count == 0 || bulk_size == 0) {
            throw std::runtime_error("Attempting to construct a exposed_memory "
                                     "descriptor from an invalid bulk handle");
        }

        void** ptrs = reinterpret_cast<void**>(
                ::alloca(bulk_count * sizeof(void*)));
        hg_size_t* sizes = reinterpret_cast<hg_size_t*>(
                ::alloca(bulk_size * sizeof(hg_size_t)));

        hg_uint32_t actual_count = 0;

        hg_return_t ret = HG_Bulk_access(bulk_handle, 
                                         static_cast<hg_size_t>(0), 
                                         bulk_size,
                                         HG_BULK_READ_ONLY,
                                         bulk_count,
                                         ptrs,
                                         sizes,
                                         &actual_count);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to construct exposed_memory "
                                     "descriptor");
        }

        assert(bulk_count == actual_count);

        DEBUG2("HG_Bulk_access(handle={}, offset={}, size={}, flags={}, "
               "max_count={}, buf_ptrs={}, buf_sizes={}, actual_count={}) = {}",
               fmt::ptr(bulk_handle), 0, bulk_size, "HG_BULK_READ_ONLY",
               bulk_count, fmt::ptr(ptrs), fmt::ptr(sizes),
               fmt::ptr(&actual_count), ret);


        DEBUG2("Bulk segments found: {{");
        for(std::size_t i = 0; i < bulk_count; ++i) {
            DEBUG2("  [addr: {}, size: {}]", ptrs[i], sizes[i]);
            m_buffers.emplace_back(ptrs[i], sizes[i]);
        }
        DEBUG2("}}");

        ///XXX ???? m_hg_class = hg_class;
        m_mode = access_mode::read_only,
        m_size = bulk_size;
    }

    /** Destroys a @c exposed_memory object */
    ~exposed_memory() { }

    /** Allows explicit conversion between a @c exposed_memory instance and a 
     * @hg_bulk_t type (useful for serializing) */
    explicit operator hg_bulk_t() {

        hg_bulk_t bulk_handle;

        void** ptrs = reinterpret_cast<void**>(
                ::alloca(m_buffers.size() * sizeof(void*)));
        hg_size_t* sizes = reinterpret_cast<hg_size_t*>(
                ::alloca(m_buffers.size() * sizeof(hg_size_t)));

        std::size_t i = 0;
        for(auto&& buf : m_buffers) {
            ptrs[i] = buf.data();
            sizes[i] = buf.size();
            ++i;
        }

        hg_return_t ret = HG_Bulk_create(
                const_cast<hg_class_t*>(m_hg_class),
                m_buffers.size(),
                ptrs,
                sizes,
                static_cast<hg_uint32_t>(m_mode),
                &bulk_handle);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to create bulk_handle "
                                     "for exposed memory");
        }

        return bulk_handle;
    }

    /** Returns the nummber of memory regions exposed */
    std::size_t
    count() const {
        return m_buffers.size();
    }

    /** Returns the accumulated size of the memory regions exposed */
    std::size_t
    size() const {
        return m_size;
    }

    iterator
    begin() {
        return m_buffers.begin();
    }

    iterator
    end() {
        return m_buffers.end();
    }

private:
    const hg_class_t * m_hg_class;
    access_mode m_mode;
    std::size_t m_size;
    std::vector<mutable_buffer> m_buffers;
};

#if 0
void push(exposed_memory src, exposed_memory dst) {
    (void) src;
    (void) dst;

}

// TODO: remove
template <typename Callable>
void pull(const request& req,
          const exposed_memory& src, 
          const exposed_memory& dst, 
          Callable&& user_callback) {

    (void) src;
    (void) dst;

    assert(src.count() == dst.count());

    void** ptrs = reinterpret_cast<void**>(
            ::alloca(dst.m_buffers.size() * sizeof(void*)));
    hg_size_t* sizes = reinterpret_cast<hg_size_t*>(
            ::alloca(dst.m_buffers.size() * sizeof(hg_size_t)));

    std::size_t i = 0;
    for(auto&& buf : dst.m_buffers) {
        ptrs[i] = buf.data();
        sizes[i] = buf.size();
        ++i;
    }

    hg_bulk_t bulk_handle;

    // create bulk handle
    hg_return_t ret = HG_Bulk_create(
                // pointer to Mercury class
                const_cast<hg_class_t*>(dst.m_hg_class),
                // number of segments
                i,
                // array of pointers to segments 
                ptrs,
                // array of segment sizes
                sizes,
                // permission flag
                HG_BULK_WRITE_ONLY,
                // pointer to returned bulk handle
                &bulk_handle);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to create bulk handle");
    }

    DEBUG2("HG_Bulk_create(hg_class={}, count={}, buf_ptrs={}, buf_sizes={}, "
           "flags={}, handle={}) = {}", fmt::ptr(dst.m_hg_class),
           i, fmt::ptr(ptrs), fmt::ptr(sizes), "HG_BULK_WRITE_ONLY",
           fmt::ptr(&bulk_handle), ret);

    struct transfer_context {
        transfer_context(Callable&& user_callback) :
            m_user_callback(user_callback) { }
        Callable m_user_callback;
    };

    const auto ctx = 
        new transfer_context(std::forward<Callable>(user_callback));

    const auto completion_callback =
            [](const struct hg_cb_info* cbi) -> hg_return_t {

                const auto* ctx = 
                    reinterpret_cast<transfer_context*>(cbi->arg);
                ctx->m_user_callback();

                delete ctx;

                return HG_SUCCESS;
            };

    const struct hg_info* hgi = HG_Get_info(req.m_handle);

    if(hgi == NULL) {
        throw std::runtime_error("Failed to retrieve request information "
                                 "from internal handle");
    }

    // initiate bulk transfer from client to server
    ret = HG_Bulk_transfer(
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
            req.m_bulk_handle,
            // origin offset
            0,
            // local bulk handle
            bulk_handle,
            // local offset
            0,
            // size of data to be transferred
            src.size(),
            // pointer to returned operation ID
            HG_OP_ID_IGNORE);

    if(ret != HG_SUCCESS) {
        throw std::runtime_error("Failed to pull remote data");
    }

    DEBUG2("HG_Bulk_transfer(hg_context={}, callback={}, arg={}, op={}, "
           "addr={}, origin_handle={}, origin_offset={}, local_handle={}, "
           "local_offset={}, size={}, HG_OP_ID_IGNORE) = {}", 
           fmt::ptr(hgi->context),
           "<cannot print>", fmt::ptr(ctx), "HG_BULK_PULL", 
           fmt::ptr(hgi->addr), fmt::ptr(&req.m_bulk_handle), 0, 
           fmt::ptr(&bulk_handle), 0, src.size(), ret);
}
#endif

} // namespace hermes


#endif // __HERMES_BULK_HPP__
