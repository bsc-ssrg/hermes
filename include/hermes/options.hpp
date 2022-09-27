#ifndef __HERMES_OPTION_HPP__
#define __HERMES_OPTION_HPP__

namespace hermes {

enum __engine_opts {
    __none             = 0,
    __use_auto_sm      = 1L << 0,
    __print_stats      = 1L << 1,
    __force_no_block_progress = 1L << 2,
    __process_may_fork = 1L << 3,
    __engine_opts_end = 1L << 16,
    __engine_opts_max = __INT_MAX__,
    __engine_opts_min = ~__INT_MAX__
};

inline constexpr __engine_opts
operator&(__engine_opts __a, __engine_opts __b) { 
    return __engine_opts(static_cast<int>(__a) & static_cast<int>(__b)); 
}

inline constexpr __engine_opts
operator|(__engine_opts __a, __engine_opts __b) { 
    return __engine_opts(static_cast<int>(__a) | static_cast<int>(__b)); 
}

inline constexpr __engine_opts
operator^(__engine_opts __a, __engine_opts __b) { 
    return __engine_opts(static_cast<int>(__a) ^ static_cast<int>(__b)); 
}

inline constexpr __engine_opts
operator~(__engine_opts __a) { 
    return __engine_opts(~static_cast<int>(__a)); 
}

inline const __engine_opts&
operator|=(__engine_opts& __a, __engine_opts __b) { 
    return __a = __a | __b; 
}

inline const __engine_opts&
operator&=(__engine_opts& __a, __engine_opts __b) { 
    return __a = __a & __b; 
}

inline const __engine_opts&
operator^=(__engine_opts& __a, __engine_opts __b) { 
    return __a = __a ^ __b; 
}

using engine_options = __engine_opts;

static const constexpr engine_options none = __engine_opts::__none;
static const constexpr engine_options use_auto_sm = __engine_opts::__use_auto_sm;
static const constexpr engine_options force_no_block_progress = __engine_opts::__force_no_block_progress;
static const constexpr engine_options print_stats = __engine_opts::__print_stats;
static const constexpr engine_options process_may_fork = __engine_opts::__process_may_fork;
} // namespace hermes

#endif // __HERMES_OPTION_HPP__
