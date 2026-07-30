#pragma once

#include "core/types.hpp"
#include <array>

namespace gb {

class Palette {
public:
    // Classic DMG Pocket/Original Color Shades
    static constexpr Color DMG_PALETTE_CLASSIC[4] = {
        {0xE0, 0xF8, 0xD0, 255}, // White / Lightest Green
        {0x88, 0xC0, 0x70, 255}, // Light Green
        {0x34, 0x68, 0x56, 255}, // Dark Green
        {0x08, 0x18, 0x20, 255}  // Black / Darkest Green
    };

    static constexpr Color DMG_PALETTE_GRAYSCALE[4] = {
        {0xFF, 0xFF, 0xFF, 255}, // White
        {0xAA, 0xAA, 0xAA, 255}, // Light Gray
        {0x55, 0x55, 0x55, 255}, // Dark Gray
        {0x00, 0x00, 0x00, 255}  // Black
    };

    static Color get_dmg_color(u8 palette_reg, u8 color_index, bool grayscale = false) {
        u8 shade = (palette_reg >> (color_index * 2)) & 0x03;
        return grayscale ? DMG_PALETTE_GRAYSCALE[shade] : DMG_PALETTE_CLASSIC[shade];
    }

    // Converts CGB 15-bit BGR555 to RGBA Color
    static Color cgb_15bit_to_color(u16 rgb555) {
        u8 r = (rgb555 & 0x1F);
        u8 g = ((rgb555 >> 5) & 0x1F);
        u8 b = ((rgb555 >> 10) & 0x1F);

        // Convert 5-bit to 8-bit color space with correct curve
        u8 r8 = (r * 13 + g * 2 + b * 1) >> 1;
        u8 g8 = (g * 12 + b * 4) >> 1;
        u8 b8 = (r * 3 + g * 2 + b * 11) >> 1;

        return Color{r8, g8, b8, 255};
    }
};

} // namespace gb
