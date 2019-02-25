#ifndef __HERMES_TRANSPORT_HPP__
#define __HERMES_TRANSPORT_HPP__

#include <cstddef>
#include <stdexcept>
#include <unordered_map>

namespace hermes {

/** Valid transport types (i.e. transport types supported by Mercury) */
enum class transport : std::size_t {

    // BMI plugin
    bmi_tcp = 0,

    // MPI plugin
    mpi_dynamic,
    mpi_static,

    // Shared memory plugin
    na_sm,

    // CCI plugin
    cci_tcp,
    cci_verbs,
    cci_gni,
    cci_sm,

    // OFI plugin
    ofi_tcp,
    ofi_verbs,
    ofi_psm2,
    ofi_gni,

    // special value: MUST ALWAYS BE LAST!
    // (it's used when defining the supported_transports constexpr std::array
    count
};

using transport_metadata = 
    std::tuple<transport, const char* const, const char* const>;

static const constexpr
std::array<
    transport_metadata,
    static_cast<std::size_t>(transport::count)
> supported_transports = {

    // BMI plugin
    std::make_tuple(
            transport::bmi_tcp,
            "bmi+tcp://",
            "bmi+tcp://"
            ),

    // MPI plugin
    std::make_tuple(
            transport::mpi_dynamic,
            "mpi+dynamic://",
            "mpi+dynamic://"
            ),
    std::make_tuple(
            transport::mpi_static,
            "mpi+static://",
            "mpi+static://"
            ),

    // Shared memory plugin
    std::make_tuple(
            transport::na_sm,
            "na+sm://",
            "na+sm://"
            ),

    // CCI plugin
    std::make_tuple(
            transport::cci_tcp,
            "cci+tcp://",
            "cci+tcp://"
            ),
    std::make_tuple(
            transport::cci_verbs,
            "cci+verbs://",
            "cci+verbs://"
            ),
    std::make_tuple(
            transport::cci_gni,
            "cci+gni://",
            "cci+gni://"
            ),
    std::make_tuple(
            transport::cci_sm,
            "cci+sm://",
            "cci+sm://"
            ),

    // OFI plugin
    std::make_tuple(
            transport::ofi_tcp,
            "ofi+sockets://",
            "ofi+sockets://"
            ),
    std::make_tuple(
            transport::ofi_verbs,
            "ofi+verbs://",
            "ofi+verbs;ofi_rxm://fi_sockaddr_in://"
            ),
    std::make_tuple(
            transport::ofi_psm2,
            "ofi+psm2://",
            "ofi+psm2://fi_addr_psmx2://"
            ),
    std::make_tuple(
            transport::ofi_gni,
            "ofi+gni://",
            "ofi+gni://fi_addr_gni://"
            )
};

} // namespace hermes

namespace {

static constexpr std::size_t
mid(const std::size_t low, const std::size_t high) {
    return (low & high) + ((low ^ high) >> 1);
}

template <std::size_t SIZE>
static constexpr std::size_t
find_transport(const std::array<hermes::transport_metadata, SIZE>& transports,
               const hermes::transport key,
               const std::size_t low = 0,
               const std::size_t high = SIZE) {

    return (high < low) ?
           throw std::logic_error("Invalid transport") :
           (key == std::get<0>(transports[::mid(low, high)])) ?
               ::mid(low, high) :
               (key < std::get<0>(transports[::mid(low, high)])) ?
               find_transport(transports, key, low, ::mid(low, high) - 1) :
               find_transport(transports, key, ::mid(low, high) + 1, high);
}

} // anonymous namespace

namespace hermes {

static constexpr const char*
get_transport_prefix(transport id) {
    return std::get<1>(
            supported_transports[
                ::find_transport(supported_transports, id)]);
}

static constexpr const char*
get_transport_lookup_prefix(transport id) {
    return std::get<2>(
            supported_transports[
                ::find_transport(supported_transports, id)]);
}

static inline transport
get_transport_type(const std::string& prefix) {

    for(auto&& t : hermes::supported_transports) {
        if(std::string(std::get<1>(t)).find(prefix) != std::string::npos) {
            return std::get<0>(t);
        }
    }

    throw std::runtime_error("Invalid transport prefix");
}

} // namespace hermes

#endif // __HERMES_TRANSPORT_HPP__
