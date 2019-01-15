#ifndef __HERMES_MAKE_UNIQUE_HPP__
#define __HERMES_MAKE_UNIQUE_HPP__

#include <memory>

namespace hermes {
namespace compat {

#if __cplusplus == 201103L

// since C++11 doesn't offer make_unique, we borrow the template definition
// from C++14 (see https://isocpp.org/files/papers/N3656.txt)
template<class T> struct _Unique_if {
    typedef std::unique_ptr<T> _Single_object;
};

template<class T> struct _Unique_if<T[]> {
    typedef std::unique_ptr<T[]> _Unknown_bound;
};

template<class T, size_t N> struct _Unique_if<T[N]> {
    typedef void _Known_bound;
};

template<class T, class... Args>
typename _Unique_if<T>::_Single_object
make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T>
typename _Unique_if<T>::_Unknown_bound
make_unique(size_t n) {
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
}

template<class T, class... Args>
typename _Unique_if<T>::_Known_bound
make_unique(Args&&...) = delete;

#else // __cplusplus != 201103L

// in C++14/17 we can simply provide an alias to the standard function
template <typename T, typename... Args>
constexpr auto make_unique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

#endif // __cplusplus != 201103L

} // namespace compat
} // namespace hermes

#endif // __HERMES_MAKE_UNIQUE_HPP__
