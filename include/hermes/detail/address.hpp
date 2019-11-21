#ifndef __HERMES_DETAIL_ADDRESS_HPP__
#define __HERMES_DETAIL_ADDRESS_HPP__

// C includes
#include <mercury.h>

namespace hermes {
namespace detail {

inline std::string
mercury_address_to_string(const hg_class_t* hg_class, const hg_addr_t address);

/** A simple RAII wrapper for hg_addr_t. This way we can keep track of 
 * generated mercury addresses both in enddpoints and in the address cache 
 * using std::shared_ptr<address>(), and only free them when the last referrer
 * dies, which is convenient */
struct address {

    static address
    self_address(const hg_class_t* hg_class) {

        hg_addr_t self_addr;
        hg_return_t ret = 
            HG_Addr_self(const_cast<hg_class_t*>(hg_class), &self_addr);

        if(ret != HG_SUCCESS) {
            throw std::runtime_error("Failed to retrieve self address: " + 
                    std::string(HG_Error_to_string(ret)));
        }

        return {hg_class, self_addr};
    }

    address() :
        m_hg_class(NULL),
        m_hg_addr(HG_ADDR_NULL) {}

    address(const hg_class_t* hg_class, 
            hg_addr_t hg_addr) :
        m_hg_class(hg_class),
        m_hg_addr(hg_addr) {}

    address(address&& rhs) : 
        m_hg_class(std::move(rhs.m_hg_class)),
        m_hg_addr(std::move(rhs.m_hg_addr)) {

        rhs.m_hg_class = NULL;
        rhs.m_hg_addr = HG_ADDR_NULL;
    }

    address& 
    operator=(address&& rhs) {
        
        if(this != &rhs) {
            m_hg_class = std::move(rhs.m_hg_class);
            m_hg_addr = std::move(rhs.m_hg_addr);

            rhs.m_hg_class = NULL;
            rhs.m_hg_addr = HG_ADDR_NULL;
        }

        return *this;
    }

    ~address() {
        if(m_hg_class != NULL && m_hg_addr != HG_ADDR_NULL) {
            HG_Addr_free(const_cast<hg_class_t*>(m_hg_class), m_hg_addr);
        }
    }

    hg_addr_t
    mercury_address() const {
        return m_hg_addr;
    }

    std::string
    to_string() const {
        try {
            return detail::mercury_address_to_string(m_hg_class, m_hg_addr);
        }
        catch(const std::exception& ex) {
            return "[[unknown_address]]";
        }
    }

    const hg_class_t* m_hg_class;
    hg_addr_t m_hg_addr;
};

} // namespace detail
} // namespace hermes

#endif // __HERMES_DETAIL_ADDRESS_HPP__
