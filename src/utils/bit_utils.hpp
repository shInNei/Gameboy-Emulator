#pragma once

#include "core/types.hpp"
#include <concepts>

namespace gb::bit {

template <std::integral T>
constexpr bool test(T val, u8 bit) {
    return (val & (T(1) << bit)) != 0;
}

template <std::integral T>
constexpr void set(T& val, u8 bit) {
    val |= (T(1) << bit);
}

template <std::integral T>
constexpr void clear(T& val, u8 bit) {
    val &= ~(T(1) << bit);
}

template <std::integral T>
constexpr void assign(T& val, u8 bit, bool state) {
    if (state) set(val, bit);
    else clear(val, bit);
}

constexpr u16 combine(u8 high, u8 low) {
    return (static_cast<u16>(high) << 8) | low;
}

constexpr u8 high(u16 word) {
    return static_cast<u8>(word >> 8);
}

constexpr u8 low(u16 word) {
    return static_cast<u8>(word & 0xFF);
}

} // namespace gb::bit
