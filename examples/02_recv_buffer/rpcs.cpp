#include <hermes.hpp>
#include "rpcs.hpp"

namespace hermes { namespace detail {

//==============================================================================
// register request types so that they can be used by users and the engine
//
void
register_user_request_types() {
    (void) registered_requests().add<example_rpcs::recv_buffer>();
    (void) registered_requests().add<example_rpcs::shutdown>();
}

}} // namespace hermes::detail
