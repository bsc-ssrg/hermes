#ifndef __HERMES_MESSAGES_HPP__
#define __HERMES_MESSAGES_HPP__

namespace hermes {

/** Message types supported by the engine. We need to use tags rather than 
 * an enum so that the type deduction machinery generates the appropirate 
 * processing functions */
struct message {
    struct simple { };
    struct bulk { };
};

} // namespace hermes

#endif // __HERMES_MESSAGES_HPP__
