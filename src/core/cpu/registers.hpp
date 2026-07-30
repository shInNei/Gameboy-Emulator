#pragma once

#include "core/types.hpp"
#include "utils/bit_utils.hpp"

namespace gb {

struct Flags {
    static constexpr u8 Z_BIT = 7; // Zero Flag
    static constexpr u8 N_BIT = 6; // Subtract Flag
    static constexpr u8 H_BIT = 5; // Half-Carry Flag
    static constexpr u8 C_BIT = 4; // Carry Flag
};

struct Registers {
    u8 a{0x01};
    u8 f{0xB0};
    u8 b{0x00};
    u8 c{0x13};
    u8 d{0x00};
    u8 e{0xD8};
    u8 h{0x01};
    u8 l{0x4D};

    u16 sp{0xFFFE};
    u16 pc{0x0100};

    // 16-bit register getters/setters
    [[nodiscard]] u16 af() const { return bit::combine(a, f & 0xF0); }
    [[nodiscard]] u16 bc() const { return bit::combine(b, c); }
    [[nodiscard]] u16 de() const { return bit::combine(d, e); }
    [[nodiscard]] u16 hl() const { return bit::combine(h, l); }

    void af(u16 val) { a = bit::high(val); f = bit::low(val) & 0xF0; }
    void bc(u16 val) { b = bit::high(val); c = bit::low(val); }
    void de(u16 val) { d = bit::high(val); e = bit::low(val); }
    void hl(u16 val) { h = bit::high(val); l = bit::low(val); }

    // Flag Accessors
    [[nodiscard]] bool get_z() const { return bit::test(f, Flags::Z_BIT); }
    [[nodiscard]] bool get_n() const { return bit::test(f, Flags::N_BIT); }
    [[nodiscard]] bool get_h() const { return bit::test(f, Flags::H_BIT); }
    [[nodiscard]] bool get_c() const { return bit::test(f, Flags::C_BIT); }

    void set_z(bool val) { bit::assign(f, Flags::Z_BIT, val); }
    void set_n(bool val) { bit::assign(f, Flags::N_BIT, val); }
    void set_h(bool val) { bit::assign(f, Flags::H_BIT, val); }
    void set_c(bool val) { bit::assign(f, Flags::C_BIT, val); }

    void set_flags(bool z, bool n, bool h, bool c) {
        set_z(z);
        set_n(n);
        set_h(h);
        set_c(c);
    }
};

} // namespace gb
