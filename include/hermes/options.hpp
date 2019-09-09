#ifndef __HERMES_OPTION_HPP__
#define __HERMES_OPTION_HPP__

namespace hermes {

enum class __engine_flags : unsigned long {
    use_auto_sm = 1L << 0,
    print_stats = 2L << 0
};

using engine_flags = __engine_flags;

static const constexpr engine_flags use_auto_sm = __engine_flags::use_auto_sm;
static const constexpr engine_flags print_stats = __engine_flags::print_stats;

} // namespace hermes

#endif // __HERMES_OPTION_HPP__
