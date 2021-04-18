#ifndef __HERMES_REQUEST_REGISTRAR_HPP__
#define __HERMES_REQUEST_REGISTRAR_HPP__

// C++ includes
#include <unordered_map>
#include <memory>

// hermes includes
#include <hermes/detail/request_descriptor.hpp>
#include <hermes/logging.hpp>

namespace hermes {
namespace detail {

template <typename Key, typename Value>
class request_registrar {

    using map_t = std::unordered_map<Key, std::shared_ptr<Value>>;

public:
    using const_iterator = typename map_t::const_iterator;

    static request_registrar<Key, Value>&
    singleton() {
        static request_registrar<Key, Value> instance;
        return instance;
    }

    template <typename Request>
    bool
    add() {
        constexpr const uint64_t id = Request::public_id;
        constexpr const hg_id_t mercury_id = Request::mercury_id;
        constexpr const auto name = Request::name;
        constexpr const auto requires_response = Request::requires_response;
        constexpr const auto mercury_in_proc_cb = Request::mercury_in_proc_cb;
        constexpr const auto mercury_out_proc_cb = Request::mercury_out_proc_cb;

        HERMES_DEBUG2("Adding new request type (id={}, name={})", id, name);

        if(m_request_types.count(id) != 0) {
            // if the request type we want to add is identical to the one
            // already registered, just ignore the request
            const auto& r = m_request_types.at(id);

            if(mercury_id == r->m_mercury_id && name == r->m_name &&
               requires_response == r->m_requires_response &&
               mercury_in_proc_cb == r->m_mercury_input_cb &&
               mercury_out_proc_cb == r->m_mercury_output_cb) {
                return true;
            }

            throw std::runtime_error("Failed to add request type: duplicate id");
        }

        m_request_types.emplace(id,
                std::make_shared<request_descriptor<Request>>(
                    id, mercury_id, name, requires_response,
                    mercury_in_proc_cb, mercury_out_proc_cb));

        return true;
    }

    std::shared_ptr<request_descriptor_base>
    at(uint64_t id) const {

        HERMES_DEBUG2("Fetching request descriptor for request type [{}]", id);

        const auto it = m_request_types.find(id);

        if(it != m_request_types.end()) {
            return m_request_types.at(id);
        }

        return {};
    }

    const_iterator
    begin() const {
        return m_request_types.begin();
    }

    const_iterator
    end() const {
        return m_request_types.end();
    }

private:
    map_t m_request_types;
};

static request_registrar<uint64_t, request_descriptor_base>&
registered_requests() {
    return request_registrar<uint64_t, request_descriptor_base>::singleton();
}

} // namespace detail
} // namespace hermes

#endif // __HERMES_REQUEST_REGISTRAR_HPP__
