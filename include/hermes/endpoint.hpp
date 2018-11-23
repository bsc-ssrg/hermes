#ifndef __HERMES_ENDPOINT_HPP__
#define __HERMES_ENDPOINT_HPP__

#include <cstddef>
#include <string>
#include <memory>

namespace hermes {

namespace detail {

// defined elsewhere
struct address;

} // namespace detail

/** */
class endpoint {

    friend class async_engine;

    endpoint(const std::string& name,
             std::size_t port,
             const std::shared_ptr<detail::address>& address) :
        m_name(name),
        m_port(port),
        m_address(address) {}

public:
    const std::string
    name() const {
        return m_name;
    }

    std::size_t
    port() const {
        return m_port;
    }

private:
    std::shared_ptr<detail::address>
    address() const {
        return m_address;
    }

private:
    const std::string m_name;
    const std::size_t m_port;
    const std::shared_ptr<detail::address> m_address;
}; // class endpoint

} // namespace hermes

#endif // __HERMES_ENDPOINT_HPP__
