#ifndef __HERMES_DETAIL_REQUEST_STATUS_HPP__
#define __HERMES_DETAIL_REQUEST_STATUS_HPP__

namespace hermes {
namespace detail {

enum class request_status {
    created,
    failed,
    timeout,
    cancelled
};

} // namespace detail
} // namespace hermes


#endif // __HERMES_DETAIL_REQUEST_STATUS_HPP__
