#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <variant>

namespace gb {

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using Address = u16;
using Cycles  = u64;

// Game Boy Display Constants
constexpr int SCREEN_WIDTH  = 160;
constexpr int SCREEN_HEIGHT = 144;
constexpr int SCREEN_SCALE  = 4;

// Clock Frequencies
constexpr u32 CPU_CLOCK_HZ         = 4194304; // 4.194304 MHz (DMG Speed)
constexpr u32 CGB_DOUBLE_CLOCK_HZ  = 8388608; // 8.388608 MHz (CGB Double Speed)
constexpr int CYCLES_PER_FRAME     = 70224;   // 59.73 Hz

// Memory Map Bounds
constexpr Address ROM_BANK_0_START = 0x0000;
constexpr Address ROM_BANK_0_END   = 0x3FFF;
constexpr Address ROM_BANK_N_START = 0x4000;
constexpr Address ROM_BANK_N_END   = 0x7FFF;
constexpr Address VRAM_START       = 0x8000;
constexpr Address VRAM_END         = 0x9FFF;
constexpr Address SRAM_START       = 0xA000;
constexpr Address SRAM_END         = 0xBFFF;
constexpr Address WRAM_BANK_0_START= 0xC000;
constexpr Address WRAM_BANK_0_END  = 0xCFFF;
constexpr Address WRAM_BANK_N_START= 0xD000;
constexpr Address WRAM_BANK_N_END  = 0xDFFF;
constexpr Address ECHO_RAM_START   = 0xE000;
constexpr Address ECHO_RAM_END     = 0xFDFF;
constexpr Address OAM_START        = 0xFE00;
constexpr Address OAM_END          = 0xFE9F;
constexpr Address UNUSABLE_START   = 0xFEA0;
constexpr Address UNUSABLE_END     = 0xFEFF;
constexpr Address IO_START         = 0xFF00;
constexpr Address IO_END           = 0xFF7F;
constexpr Address HRAM_START       = 0xFF80;
constexpr Address HRAM_END         = 0xFFFE;
constexpr Address IE_REGISTER      = 0xFFFF;

// RGBA Color Struct for Framebuffer
struct Color {
    u8 r{0}, g{0}, b{0}, a{255};

    constexpr bool operator==(const Color& other) const = default;
};

using Framebuffer = std::array<Color, SCREEN_WIDTH * SCREEN_HEIGHT>;

} // namespace gb
