#ifndef __HERMES_TRANSPORT_HPP__
#define __HERMES_TRANSPORT_HPP__

#include <cstddef>
#include <unordered_map>

namespace hermes {

/** Valid transport types (i.e. transport types supported by Mercury) */
enum class transport {
    bmi_tcp = 0, mpi_dynamic, mpi_static, na_sm, cci_tcp, cci_verbs, cci_gni,
    cci_sm, ofi_tcp, ofi_psm2, ofi_verbs, ofi_gni
};

/** enum class hash (required for upcoming std::unordered_maps */
struct enum_class_hash {
    template <typename T>
    std::size_t operator()(T t) const noexcept {
        return static_cast<std::size_t>(t);
    }
};

/** Map to translate from transport to Mercury's transport string */
static const
std::unordered_map<
    transport,
    const char* const,
    enum_class_hash
> known_transports = {
    { transport::bmi_tcp,     "bmi+tcp://"     },
    { transport::mpi_dynamic, "mpi+dynamic://" },
    { transport::mpi_static,  "mpi+static://"  },
    { transport::na_sm,       "na+sm://"       },
    { transport::cci_tcp,     "cci+tcp://"     },
    { transport::cci_verbs,   "cci+verbs://"   },
    { transport::cci_gni,     "cci+gni://"     },
    { transport::cci_sm,      "cci+sm://"      },
    { transport::ofi_tcp,     "ofi+tcp://"     },
    { transport::ofi_psm2,    "ofi+psm2://"    },
    { transport::ofi_verbs,   "ofi+verbs://"   },
    { transport::ofi_gni,     "ofi+gni://"     }
};

} // namespace hermes

#endif // __HERMES_TRANSPORT_HPP__
