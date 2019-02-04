#ifndef __HERMES_ACCESS_MODE_HPP__
#define __HERMES_ACCESS_MODE_HPP__

#include <mercury.h>

namespace hermes {

enum class access_mode : hg_uint32_t {
    read_only  = HG_BULK_READ_ONLY,
    write_only = HG_BULK_WRITE_ONLY,
    read_write = HG_BULK_READWRITE
};

} // namespace hermes

#endif // __HERMES_ACCESS_MODE_HPP__
