#pragma once

#include "core/types.hpp"
#include "utils/bit_utils.hpp"

namespace gb {

enum class PPUMode : u8 {
    HBlank = 0,
    VBlank = 1,
    OAMSearch = 2,
    PixelTransfer = 3
};

struct LCDControl {
    u8 reg{0x91};

    [[nodiscard]] bool lcd_enable() const { return bit::test(reg, 7); }
    [[nodiscard]] Address window_tile_map() const { return bit::test(reg, 6) ? 0x9C00 : 0x9800; }
    [[nodiscard]] bool window_enable() const { return bit::test(reg, 5); }
    [[nodiscard]] Address bg_window_tile_data() const { return bit::test(reg, 4) ? 0x8000 : 0x8800; }
    [[nodiscard]] Address bg_tile_map() const { return bit::test(reg, 3) ? 0x9C00 : 0x9800; }
    [[nodiscard]] int sprite_height() const { return bit::test(reg, 2) ? 16 : 8; }
    [[nodiscard]] bool sprite_enable() const { return bit::test(reg, 1); }
    [[nodiscard]] bool bg_window_enable() const { return bit::test(reg, 0); }
};

struct LCDStatus {
    u8 reg{0x85};

    [[nodiscard]] bool lyc_ly_interrupt() const { return bit::test(reg, 6); }
    [[nodiscard]] bool mode2_oam_interrupt() const { return bit::test(reg, 5); }
    [[nodiscard]] bool mode1_vblank_interrupt() const { return bit::test(reg, 4); }
    [[nodiscard]] bool mode0_hblank_interrupt() const { return bit::test(reg, 3); }

    [[nodiscard]] bool lyc_equals_ly() const { return bit::test(reg, 2); }
    void set_lyc_equals_ly(bool val) { bit::assign(reg, 2, val); }

    [[nodiscard]] PPUMode mode() const { return static_cast<PPUMode>(reg & 0x03); }
    void set_mode(PPUMode mode) { reg = (reg & 0xFC) | static_cast<u8>(mode); }
};

} // namespace gb
