#ifndef __HERMES_MAKE_UNIQUE_HPP__
#define __HERMES_MAKE_UNIQUE_HPP__

#ifndef HERMES_DISABLE_INTERNAL_MAKE_UNIQUE

#include <memory>

#if __cplusplus != 201103L
#error This file should only be included when compiling in C++11 mode. \
       To safely use this file when compiling with the C++11 standard, \
       guard it like this: \
       "#if __cplusplus == 201103L #include <hermes/make_unique.hpp #endif"
#endif

// since C++11 doesn't offer make_unique, we borrow the template definition
// from C++14 (see https://isocpp.org/files/papers/N3656.txt)
namespace std {
    template<class T> struct _Unique_if {
        typedef unique_ptr<T> _Single_object;
    };

    template<class T> struct _Unique_if<T[]> {
        typedef unique_ptr<T[]> _Unknown_bound;
    };

    template<class T, size_t N> struct _Unique_if<T[N]> {
        typedef void _Known_bound;
    };

    template<class T, class... Args>
        typename _Unique_if<T>::_Single_object
        make_unique(Args&&... args) {
            return unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

    template<class T>
        typename _Unique_if<T>::_Unknown_bound
        make_unique(size_t n) {
            typedef typename remove_extent<T>::type U;
            return unique_ptr<T>(new U[n]());
        }

    template<class T, class... Args>
        typename _Unique_if<T>::_Known_bound
        make_unique(Args&&...) = delete;
}

#endif // HERMES_DISABLE_INTERNAL_MAKE_UNIQUE

#endif // __HERMES_MAKE_UNIQUE_HPP__
