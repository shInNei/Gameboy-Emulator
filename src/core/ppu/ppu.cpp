#include "ppu.hpp"
#include "utils/bit_utils.hpp"
#include <algorithm>

namespace gb {

void PPU::reset() {
    for (auto& bank : vram) bank.fill(0);
    oam.fill(0);

    front_buffer.fill(Color{255, 255, 255, 255});
    back_buffer.fill(Color{255, 255, 255, 255});
    frame_ready = false;

    control.reg = 0x91;
    status.reg = 0x85;
    scy = 0;
    scx = 0;
    ly = 0;
    lyc = 0;
    bgp = 0xFC;
    obp0 = 0xFF;
    obp1 = 0xFF;
    wy = 0;
    wx = 0;

    vram_bank_sel = 0;
    dot_counter = 0;
    window_line_counter = 0;
    window_y_triggered = false;
}

u8 PPU::read_vram(Address addr, u8 bank) const {
    size_t offset = addr - 0x8000;
    if (offset < 0x2000) {
        return vram[bank & 0x01][offset];
    }
    return 0xFF;
}

void PPU::write_vram(Address addr, u8 val, u8 bank) {
    size_t offset = addr - 0x8000;
    if (offset < 0x2000) {
        vram[bank & 0x01][offset] = val;
    }
}

u8 PPU::read_oam(Address addr) const {
    size_t offset = addr - 0xFE00;
    if (offset < 0xA0) return oam[offset];
    return 0xFF;
}

void PPU::write_oam(Address addr, u8 val) {
    size_t offset = addr - 0xFE00;
    if (offset < 0xA0) oam[offset] = val;
}

u8 PPU::read_register(Address addr) const {
    switch (addr) {
        case 0xFF40: return control.reg;
        case 0xFF41: return status.reg | 0x80;
        case 0xFF42: return scy;
        case 0xFF43: return scx;
        case 0xFF44: return ly;
        case 0xFF45: return lyc;
        case 0xFF47: return bgp;
        case 0xFF48: return obp0;
        case 0xFF49: return obp1;
        case 0xFF4A: return wy;
        case 0xFF4B: return wx;
        case 0xFF4F: return vram_bank_sel | 0xFE;
        default: return 0xFF;
    }
}

void PPU::check_lyc() {
    if (!control.lcd_enable()) return;
    bool equals = (ly == lyc);
    status.set_lyc_equals_ly(equals);
    if (equals && status.lyc_ly_interrupt()) {
        interrupt_controller.request_interrupt(InterruptType::LCDStat);
    }
}

void PPU::write_register(Address addr, u8 val) {
    switch (addr) {
        case 0xFF40: {
            bool was_enabled = control.lcd_enable();
            control.reg = val;
            if (was_enabled && !control.lcd_enable()) {
                ly = 0;
                dot_counter = 0;
                window_line_counter = 0;
                window_y_triggered = false;
                change_mode(PPUMode::HBlank);
                check_lyc();
            }
            break;
        }
        case 0xFF41:
            status.reg = (status.reg & 0x07) | (val & 0xF8);
            break;
        case 0xFF42: scy = val; break;
        case 0xFF43: scx = val; break;
        case 0xFF45:
            lyc = val;
            check_lyc();
            break;
        case 0xFF47: bgp = val; break;
        case 0xFF48: obp0 = val; break;
        case 0xFF49: obp1 = val; break;
        case 0xFF4A: wy = val; break;
        case 0xFF4B: wx = val; break;
        case 0xFF4F: vram_bank_sel = val & 0x01; break;
        default: break;
    }
}

void PPU::change_mode(PPUMode new_mode) {
    status.set_mode(new_mode);

    if (!control.lcd_enable()) return;

    bool stat_irq = false;
    switch (new_mode) {
        case PPUMode::HBlank:
            if (status.mode0_hblank_interrupt()) stat_irq = true;
            break;
        case PPUMode::VBlank:
            if (status.mode1_vblank_interrupt()) stat_irq = true;
            interrupt_controller.request_interrupt(InterruptType::VBlank);
            break;
        case PPUMode::OAMSearch:
            if (status.mode2_oam_interrupt()) stat_irq = true;
            break;
        case PPUMode::PixelTransfer:
            break;
    }

    if (stat_irq) {
        interrupt_controller.request_interrupt(InterruptType::LCDStat);
    }
}

void PPU::tick(u16 t_cycles) {
    if (!control.lcd_enable()) return;

    dot_counter += t_cycles;

    switch (status.mode()) {
        case PPUMode::OAMSearch:
            if (dot_counter >= 80) {
                dot_counter -= 80;
                change_mode(PPUMode::PixelTransfer);
                if (ly < 144) {
                    render_scanline();
                }
            }
            break;

        case PPUMode::PixelTransfer:
            if (dot_counter >= 172) {
                dot_counter -= 172;
                change_mode(PPUMode::HBlank);
            }
            break;

        case PPUMode::HBlank:
            if (dot_counter >= 204) {
                dot_counter -= 204;
                ly++;
                check_lyc();

                if (ly == 144) {
                    front_buffer = back_buffer;
                    frame_ready = true;
                    change_mode(PPUMode::VBlank);
                } else {
                    change_mode(PPUMode::OAMSearch);
                }
            }
            break;

        case PPUMode::VBlank:
            if (dot_counter >= 456) {
                dot_counter -= 456;
                ly++;

                if (ly > 153) {
                    ly = 0;
                    window_line_counter = 0;
                    window_y_triggered = false;
                    check_lyc();
                    change_mode(PPUMode::OAMSearch);
                } else {
                    check_lyc();
                }
            }
            break;
    }
}

void PPU::render_scanline() {
    bg_color_indices.fill(0);

    if (ly == wy && control.window_enable()) {
        window_y_triggered = true;
    }

    if (control.bg_window_enable()) {
        render_background();
        if (control.window_enable() && window_y_triggered && wx <= 166) {
            render_window();
        }
    } else {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            back_buffer[ly * SCREEN_WIDTH + x] = Palette::get_dmg_color(bgp, 0);
        }
    }

    if (control.sprite_enable()) {
        render_sprites();
    }
}

void PPU::render_background() {
    Address map_base = control.bg_tile_map();
    Address data_base = control.bg_window_tile_data();
    bool unsigned_tiles = (data_base == 0x8000);

    u8 y_offset = scy + ly;
    u8 tile_y = y_offset / 8;
    u8 pixel_y = y_offset % 8;

    for (int x = 0; x < SCREEN_WIDTH; ++x) {
        u8 x_offset = scx + x;
        u8 tile_x = x_offset / 8;
        u8 pixel_x = x_offset % 8;

        Address map_addr = map_base + (tile_y * 32) + tile_x;
        u8 tile_id = read_vram(map_addr, 0);

        Address tile_addr;
        if (unsigned_tiles) {
            tile_addr = 0x8000 + (tile_id * 16);
        } else {
            tile_addr = 0x9000 + static_cast<i16>(static_cast<i8>(tile_id)) * 16;
        }

        u8 byte1 = read_vram(tile_addr + pixel_y * 2, 0);
        u8 byte2 = read_vram(tile_addr + pixel_y * 2 + 1, 0);

        u8 bit_pos = 7 - pixel_x;
        u8 color_idx = ((bit::test(byte2, bit_pos) ? 1 : 0) << 1) | (bit::test(byte1, bit_pos) ? 1 : 0);

        bg_color_indices[x] = color_idx;
        back_buffer[ly * SCREEN_WIDTH + x] = Palette::get_dmg_color(bgp, color_idx);
    }
}

void PPU::render_window() {
    if (wx > 166 || !window_y_triggered) return;

    int window_x_start = static_cast<int>(wx) - 7;
    Address map_base = control.window_tile_map();
    Address data_base = control.bg_window_tile_data();
    bool unsigned_tiles = (data_base == 0x8000);

    u8 tile_y = window_line_counter / 8;
    u8 pixel_y = window_line_counter % 8;

    bool window_rendered = false;

    for (int x = std::max(0, window_x_start); x < SCREEN_WIDTH; ++x) {
        window_rendered = true;
        u8 win_pixel_x = x - window_x_start;
        u8 tile_x = win_pixel_x / 8;
        u8 pixel_x = win_pixel_x % 8;

        Address map_addr = map_base + (tile_y * 32) + tile_x;
        u8 tile_id = read_vram(map_addr, 0);

        Address tile_addr;
        if (unsigned_tiles) {
            tile_addr = 0x8000 + (tile_id * 16);
        } else {
            tile_addr = 0x9000 + static_cast<i16>(static_cast<i8>(tile_id)) * 16;
        }

        u8 byte1 = read_vram(tile_addr + pixel_y * 2, 0);
        u8 byte2 = read_vram(tile_addr + pixel_y * 2 + 1, 0);

        u8 bit_pos = 7 - pixel_x;
        u8 color_idx = ((bit::test(byte2, bit_pos) ? 1 : 0) << 1) | (bit::test(byte1, bit_pos) ? 1 : 0);

        bg_color_indices[x] = color_idx;
        back_buffer[ly * SCREEN_WIDTH + x] = Palette::get_dmg_color(bgp, color_idx);
    }

    if (window_rendered) {
        window_line_counter++;
    }
}

void PPU::render_sprites() {
    int sprite_h = control.sprite_height();

    std::vector<Sprite> line_sprites;
    line_sprites.reserve(10);

    for (u8 i = 0; i < 40; ++i) {
        Sprite s;
        s.y = oam[i * 4];
        s.x = oam[i * 4 + 1];
        s.tile_id = oam[i * 4 + 2];
        s.flags = oam[i * 4 + 3];
        s.oam_index = i;

        int sprite_y = static_cast<int>(s.y) - 16;
        if (ly >= sprite_y && ly < sprite_y + sprite_h) {
            line_sprites.push_back(s);
            if (line_sprites.size() == 10) break; // Game Boy hardware limit 10 sprites per scanline
        }
    }

    // Sort sprites by X coordinate (DMG priority)
    std::stable_sort(line_sprites.begin(), line_sprites.end(), [](const Sprite& a, const Sprite& b) {
        return a.x < b.x;
    });

    // Render sprites in reverse so smaller X sprites overlap higher X sprites
    for (auto it = line_sprites.rbegin(); it != line_sprites.rend(); ++it) {
        const auto& s = *it;
        int sprite_x = static_cast<int>(s.x) - 8;
        int sprite_y = static_cast<int>(s.y) - 16;
        int line = ly - sprite_y;

        if (s.y_flip()) {
            line = sprite_h - 1 - line;
        }

        u8 tile_id = s.tile_id;
        if (sprite_h == 16) {
            tile_id &= 0xFE; // Ignore bit 0 in 8x16 mode
            if (line >= 8) {
                tile_id |= 1;
                line -= 8;
            }
        }

        Address tile_addr = 0x8000 + (tile_id * 16) + (line * 2);
        u8 byte1 = read_vram(tile_addr, 0);
        u8 byte2 = read_vram(tile_addr + 1, 0);

        for (int tile_x = 0; tile_x < 8; ++tile_x) {
            int screen_x = sprite_x + tile_x;
            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) continue;

            u8 bit_pos = s.x_flip() ? tile_x : (7 - tile_x);
            u8 color_idx = ((bit::test(byte2, bit_pos) ? 1 : 0) << 1) | (bit::test(byte1, bit_pos) ? 1 : 0);

            if (color_idx == 0) continue; // Color 0 is transparent for sprites

            // Priority check against background
            if (s.priority() && bg_color_indices[screen_x] != 0) continue;

            u8 pal_reg = s.palette_dmg() ? obp1 : obp0;
            back_buffer[ly * SCREEN_WIDTH + screen_x] = Palette::get_dmg_color(pal_reg, color_idx);
        }
    }
}

} // namespace gb
