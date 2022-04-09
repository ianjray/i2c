#pragma once

#include <type_traits>

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3485.pdf
// §17.5.2.1.3 Bitmask types
//
// Define bitwise-or operator to allow type-safe bitwise-or.
// Define bitwise-and operator to allow type-safe bit test.

#define BITMASK_OPERATORS(T) \
    inline T operator| (T a, T b) { \
        return static_cast<T>(static_cast<std::underlying_type<T>::type>(a) | static_cast<std::underlying_type<T>::type>(b) ); \
    } \
    inline bool operator& (T a, T b) { \
        return static_cast<std::underlying_type<T>::type>(a) & static_cast<std::underlying_type<T>::type>(b); \
    }
