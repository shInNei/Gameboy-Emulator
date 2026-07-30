#pragma once

#include "core/types.hpp"
#include "core/quirks.hpp"
#include "core/bus/interrupt_controller.hpp"
#include "lcd.hpp"
#include "palette.hpp"
#include <array>
#include <vector>

namespace gb {

struct Sprite {
    u8 y{0};
    u8 x{0};
    u8 tile_id{0};
    u8 flags{0};
    u8 oam_index{0};

    [[nodiscard]] bool priority() const { return bit::test(flags, 7); }
    [[nodiscard]] bool y_flip() const { return bit::test(flags, 6); }
    [[nodiscard]] bool x_flip() const { return bit::test(flags, 5); }
    [[nodiscard]] u8 palette_dmg() const { return bit::test(flags, 4) ? 1 : 0; }
    [[nodiscard]] u8 vram_bank_cgb() const { return bit::test(flags, 3) ? 1 : 0; }
    [[nodiscard]] u8 palette_cgb() const { return flags & 0x07; }
};

class PPU {
public:
    PPU(InterruptController& interrupts, const HardwareQuirks& quirks)
        : interrupt_controller(interrupts), quirks(quirks) {}

    void reset();
    void tick(u16 t_cycles);

    [[nodiscard]] u8 read_vram(Address addr, u8 bank = 0) const;
    void write_vram(Address addr, u8 val, u8 bank = 0);

    [[nodiscard]] u8 read_oam(Address addr) const;
    void write_oam(Address addr, u8 val);

    [[nodiscard]] u8 read_register(Address addr) const;
    void write_register(Address addr, u8 val);

    [[nodiscard]] const Framebuffer& framebuffer() const { return front_buffer; }
    [[nodiscard]] bool is_frame_ready() const { return frame_ready; }
    void clear_frame_ready() { frame_ready = false; }

    [[nodiscard]] const LCDControl& lcdc() const { return control; }
    [[nodiscard]] const LCDStatus& stat() const { return status; }

    [[nodiscard]] const std::array<u8, 0x2000>& vram_bank0() const { return vram[0]; }
    [[nodiscard]] const std::array<u8, 0x2000>& vram_bank1() const { return vram[1]; }
    [[nodiscard]] const std::array<u8, 0xA0>& oam_bytes() const { return oam; }

private:
    void change_mode(PPUMode new_mode);
    void check_lyc();
    void render_scanline();
    void render_background();
    void render_window();
    void render_sprites();

    InterruptController& interrupt_controller;
    const HardwareQuirks& quirks;

    std::array<std::array<u8, 0x2000>, 2> vram{}; // VRAM Bank 0 and Bank 1 (CGB)
    std::array<u8, 0xA0> oam{};                   // OAM 40 Sprites x 4 bytes

    Framebuffer front_buffer{};
    Framebuffer back_buffer{};
    bool frame_ready{false};

    LCDControl control{};
    LCDStatus status{};

    u8 scy{0};
    u8 scx{0};
    u8 ly{0};
    u8 lyc{0};
    u8 bgp{0xFC};
    u8 obp0{0xFF};
    u8 obp1{0xFF};
    u8 wy{0};
    u8 wx{0};

    u8 vram_bank_sel{0};
    u16 dot_counter{0};
    u8 window_line_counter{0};
    bool window_y_triggered{false};

    // Color buffer for pixel priority during sprite rendering
    std::array<u8, SCREEN_WIDTH> bg_color_indices{};
};

} // namespace gb
