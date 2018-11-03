#ifndef __HERMES_EVENT_HPP__
#define __HERMES_EVENT_HPP__

namespace hermes {

/** Defines identifiers for events understood by the engine that the user can
 * employ to specify when things can happen, such as when a certain RPC
 * handler should be run */
enum class event : uint16_t {
    on_arrival = 0 ,
    on_completion,
    count,
};

} // namespace hermes

#endif // __HERMES_EVENT_HPP__
