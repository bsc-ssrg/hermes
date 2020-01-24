#ifndef __HERMES_BULK_HPP__
#define __HERMES_BULK_HPP__


// C includes
#include <mercury.h>
#include <mercury_bulk.h>
#include <alloca.h>

// C++ includes
#include <cassert>

#ifdef HERMES_DEBUG_BUILD
#include <string>
#endif

// project includes
#include <hermes/access_mode.hpp>
#include <hermes/buffer.hpp>
#include <hermes/logging.hpp>
#include <hermes/detail/mercury_utils.hpp>


namespace hermes {

/**
 * The @c exposed_memory class represent abstractions of memory segments exposed
 * by a process for RDMA operations. A exposed_memory object can explicitly be
 * converted to a hg_bulk_t so that Mercury can send it over RPC to another
 * process.  */
class exposed_memory {

    friend class async_engine;

public:

    using iterator = std::vector<mutable_buffer>::iterator;
    using const_iterator = std::vector<mutable_buffer>::const_iterator;

    /** Constructs an empty exposed_memory object */
    exposed_memory() :
        m_hg_class(NULL),
        m_mode(access_mode::read_only),
        m_size(0),
        m_bulk_handle(HG_BULK_NULL) { }

    /** Constructs a @c exposed_memory object from a @c BufferSequence */
    template <typename BufferSequence>
    exposed_memory(const hg_class_t* hg_class,
                   access_mode mode,
                   BufferSequence&& bufseq) :
        m_hg_class(hg_class),
        m_mode(mode),
        m_size(0),
        m_bulk_handle(HG_BULK_NULL) {

        std::vector<void*> ptrs;
        std::vector<hg_size_t> sizes;

        m_buffers.reserve(bufseq.size());
        ptrs.reserve(bufseq.size());
        sizes.reserve(bufseq.size());

        for(auto&& buf : bufseq) {
            m_buffers.emplace_back(buf);
            ptrs.emplace_back(buf.data());
            sizes.emplace_back(buf.size());
            m_size += buf.size();
        }

        m_bulk_handle = 
            detail::create_mercury_bulk_handle(
                    const_cast<hg_class_t*>(m_hg_class),
                    m_buffers.size(),
                    ptrs.data(),
                    sizes.data(),
                    static_cast<hg_uint32_t>(m_mode));

#ifdef HERMES_DEBUG_BUILD
        HERMES_DEBUG2("{}(this={})", __func__, fmt::ptr(this));
        this->print("this");
#endif
    }

    exposed_memory(const exposed_memory& other) :
        m_hg_class(other.m_hg_class),
        m_mode(other.m_mode),
        m_size(other.m_size),
        m_bulk_handle(other.m_bulk_handle),
        m_buffers(other.m_buffers) { 

        // since Mercury keeps a reference count for bulk handles
        // we don't need to actually copy it, we can simply increase
        // it's reference count. This also simplifies the destructor.
        if(m_bulk_handle != HG_BULK_NULL) {
            HG_Bulk_ref_incr(m_bulk_handle);
        }

#ifdef HERMES_DEBUG_BUILD
        HERMES_DEBUG2("{}(this={}, other={})", 
                      __func__, fmt::ptr(this), fmt::ptr(&other));

        this->print("this");
        other.print("other");
#endif

    }

    exposed_memory&
    operator=(const exposed_memory& other) {

        if(this != &other) {
            m_hg_class = other.m_hg_class;
            m_mode = other.m_mode;
            m_size = other.m_size;
            m_bulk_handle = other.m_bulk_handle;
            m_buffers = other.m_buffers;

            // since Mercury keeps a reference count for bulk handles
            // we don't need to actually copy it, we can simply increase
            // it's reference count. This also simplifies the destructor.
            if(m_bulk_handle != HG_BULK_NULL) {
                HG_Bulk_ref_incr(m_bulk_handle);
            }
        }

#ifdef HERMES_DEBUG_BUILD
        HERMES_DEBUG2("{}(this={}, other={})", 
                      __func__, fmt::ptr(this), fmt::ptr(&other));

        this->print("this");
        other.print("other");
#endif

        return *this;
    }

    exposed_memory(exposed_memory&& rhs) :
        m_hg_class(std::move(rhs.m_hg_class)),
        m_mode(std::move(rhs.m_mode)),
        m_size(std::move(rhs.m_size)),
        m_bulk_handle(std::move(rhs.m_bulk_handle)),
        m_buffers(std::move(rhs.m_buffers)) { 

        rhs.m_hg_class = NULL;
        rhs.m_mode = access_mode::read_only;
        rhs.m_size = 0;
        rhs.m_bulk_handle = HG_BULK_NULL;

#ifdef HERMES_DEBUG_BUILD
        HERMES_DEBUG2("{}(this={}, rhs={})", 
                      __func__, fmt::ptr(this), fmt::ptr(&rhs));

        this->print("this");
        rhs.print("rhs");
#endif
    }

    exposed_memory&
    operator=(exposed_memory&& rhs) {

        if(this != &rhs) {
            m_hg_class = std::move(rhs.m_hg_class);
            m_mode = std::move(rhs.m_mode);
            m_size = std::move(rhs.m_size);
            m_bulk_handle = std::move(rhs.m_bulk_handle);
            m_buffers = std::move(rhs.m_buffers);

            rhs.m_hg_class = NULL;
            rhs.m_mode = access_mode::read_only;
            rhs.m_size = 0;
            rhs.m_bulk_handle = HG_BULK_NULL;
        }

#ifdef HERMES_DEBUG_BUILD
        HERMES_DEBUG2("{}(this={}, rhs={})", 
                      __func__, fmt::ptr(this), fmt::ptr(&rhs));

        this->print("this");
        rhs.print("rhs");
#endif

        return *this;
    }


//XXX these conversion constructors should be private
    /** Converts between a Mercury bulk handle and a exposed_memory object */
    explicit exposed_memory(hg_bulk_t bulk_handle) { 

        HERMES_DEBUG("Converting hg_bulk_t to hermes::exposed_memory");

        hg_uint32_t bulk_count = HG_Bulk_get_segment_count(bulk_handle);
        hg_size_t bulk_size = HG_Bulk_get_size(bulk_handle);

        if(bulk_count == 0 || bulk_size == 0) {
            throw std::runtime_error("Attempting to construct a exposed_memory "
                                     "descriptor from an invalid bulk handle");
        }

        void** ptrs = reinterpret_cast<void**>(
                ::alloca(bulk_count * sizeof(void*)));
        hg_size_t* sizes = reinterpret_cast<hg_size_t*>(
                ::alloca(bulk_count * sizeof(hg_size_t)));

        hg_uint32_t actual_count = 0;

        hg_return_t ret = 
            HG_Bulk_access(bulk_handle, 
                           static_cast<hg_size_t>(0), 
                           bulk_size,
                           HG_BULK_READ_ONLY, // ignored by Mercury ATM
                           bulk_count,
                           ptrs,
                           sizes,
                           &actual_count);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to construct exposed_memory "
                                     "descriptor");
        }

        assert(bulk_count == actual_count);

        HERMES_DEBUG2("HG_Bulk_access(handle={}, offset={}, size={}, flags={}, "
                      "max_count={}, buf_ptrs={}, buf_sizes={}, "
                      "actual_count={}) = {}",
                      fmt::ptr(bulk_handle), 0, bulk_size, "HG_BULK_READ_ONLY",
                      bulk_count, fmt::ptr(ptrs), fmt::ptr(sizes),
                      fmt::ptr(&actual_count), ret);

        m_buffers.reserve(bulk_count);

        HERMES_DEBUG2("Bulk segments found: {{");
        for(std::size_t i = 0; i < bulk_count; ++i) {
            HERMES_DEBUG2("  [addr: {}, size: {}]", ptrs[i], sizes[i]);
            m_buffers.emplace_back(ptrs[i], sizes[i]);
        }
        HERMES_DEBUG2("}}");

        ///XXX Should this refer to the local class or the remote class?
        // (also, Mercury does not offer a way to retrieve this info, yet)
        /// m_hg_class = hg_class;
        //XXX we would need something like HG_Bulk_get_flags() but it does
        // not exist in Mercury yet. Thus, for now we set it to read_write 
        m_mode = access_mode::read_write,
        m_size = bulk_size;
        m_bulk_handle = bulk_handle;

        // since Mercury keeps a reference count for bulk handles
        // we don't need to actually copy it, we can simply increase
        // it's reference count. This also simplifies the destructor.
        if(m_bulk_handle != HG_BULK_NULL) {
            HG_Bulk_ref_incr(m_bulk_handle);
        }
    }

    /** Destroys a @c exposed_memory object */
    ~exposed_memory() { 

        HERMES_DEBUG2("{}(this={})", __func__, fmt::ptr(this));

        if(m_bulk_handle != HG_BULK_NULL) {
            // Mercury will actually free the bulk handle only if its 
            // reference count reaches zero. Thus, we can safely invoke
            // HG_Bulk_free() and it will only free the handle when there's
            // only one instance remaining
            HG_Bulk_free(m_bulk_handle);
        }
    }

    /** Allows explicit conversion between a @c exposed_memory instance and a 
     * @hg_bulk_t type (useful for serializing) */
    explicit operator hg_bulk_t() {

        assert(m_bulk_handle != HG_BULK_NULL);

        // since Mercury keeps a reference count for bulk handles
        // we don't need to actually copy it, we can simply increase
        // it's reference count. This also simplifies the destructor.
        if(m_bulk_handle != HG_BULK_NULL) {
            HG_Bulk_ref_incr(m_bulk_handle);
        }

        return m_bulk_handle;
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

    /** Returns the associated Mercury bulk handle */
    hg_bulk_t
    mercury_bulk_handle() const {
        return m_bulk_handle;
    }

    iterator
    begin() {
        return m_buffers.begin();
    }

    iterator
    end() {
        return m_buffers.end();
    }

    const_iterator
    begin() const{
        return m_buffers.begin();
    }

    const_iterator
    end() const {
        return m_buffers.end();
    }

#ifdef HERMES_DEBUG_BUILD
    void
    print(const std::string& alias) const {

        auto get_ref_count = [](hg_bulk_t bulk_handle) -> hg_int32_t {
            hg_int32_t ref_count = -1;

            if(!bulk_handle) {
                return -1;
            }

#if 0
            hg_return_t ret = 
                HG_Bulk_ref_get(bulk_handle, &ref_count);

                if(ret != HG_SUCCESS) {
                    return -1;
                }
#endif

            return ref_count;
        };

#ifndef HAVE_FMT
        (void) alias;
        (void) get_ref_count;
#endif

        HERMES_DEBUG2("{} = {{", alias);
        HERMES_DEBUG2("  m_hg_class = {},", fmt::ptr(m_hg_class));
        HERMES_DEBUG2("  m_mode = {},", static_cast<hg_uint32_t>(m_mode));
        HERMES_DEBUG2("  m_size = {},", m_size);
        HERMES_DEBUG2("  m_bulk_handle = {}, (ref_count:{})",
                      fmt::ptr(m_bulk_handle), get_ref_count(m_bulk_handle));
        HERMES_DEBUG2("  m_buffers = {{");

        for(auto&& b : m_buffers) {
            (void) b;
            HERMES_DEBUG2("    {{data={}, size={}}},", 
                          fmt::ptr(b.data()), b.size());
        }
        HERMES_DEBUG2("  }},");
        HERMES_DEBUG2("}};");
    }
#endif

private:
    const hg_class_t * m_hg_class;
    access_mode m_mode;
    std::size_t m_size;
    hg_bulk_t m_bulk_handle;
    std::vector<mutable_buffer> m_buffers;
};

} // namespace hermes


#endif // __HERMES_BULK_HPP__
