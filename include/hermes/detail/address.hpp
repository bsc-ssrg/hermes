#ifndef __HERMES_DETAIL_ADDRESS_HPP__
#define __HERMES_DETAIL_ADDRESS_HPP__

// C includes
#include <mercury.h>

namespace hermes {
namespace detail {

/** A simple RAII wrapper for hg_addr_t. This way we can keep track of 
 * generated mercury addresses both in enddpoints and in the address cache 
 * using std::shared_ptr<address>(), and only free them when the last referrer
 * dies, which is convenient */
struct address {

    address(hg_class_t* hg_class, 
            hg_addr_t hg_addr) :
        m_hg_class(hg_class),
        m_hg_addr(hg_addr) {}

    ~address() {
        HG_Addr_free(const_cast<hg_class_t*>(m_hg_class), m_hg_addr);
    }

    hg_addr_t
    mercury_address() const {
        return m_hg_addr;
    }

    const hg_class_t* const m_hg_class;
    const hg_addr_t m_hg_addr;
};

} // namespace detail
} // namespace hermes

#endif // __HERMES_DETAIL_ADDRESS_HPP__
