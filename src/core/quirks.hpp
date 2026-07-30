#pragma once

#include "types.hpp"

namespace gb {

enum class HardwareModel {
    Auto,
    DMG_RevB,
    MGB,
    CGB_RevC,
    AGB
};

struct HardwareQuirks {
    HardwareModel model{HardwareModel::Auto};

    // CPU Quirks
    bool halt_bug{true};               // LR35902 HALT bug: PC does not increment on next opcode fetch if IE & IF != 0
    bool oam_corruption_bug{true};     // Corruption of OAM during 16-bit register access in OAM scan mode
    bool interrupt_delay{true};        // EI instruction delays interrupt enabling by 1 opcode
    bool pop_af_mask_flags{true};      // Low 4 bits of F register are always zero (F & 0xF0)

    // PPU Quirks
    bool stat_interrupt_glitch{true};  // STAT IRQ triggers on mode transitions or LCDC/STAT writes unexpectedly
    bool vblank_stat_bug{true};        // DMG Mode 1 STAT IRQ glitch
    bool lcd_enable_first_frame{true}; // First frame after LCD enabled is blank/white
    bool oam_read_during_mode2{true};  // Reading OAM in Mode 2 returns 0xFF

    // Timer Quirks
    bool tima_reload_delay{true};      // 1 M-cycle delay when TIMA reloads from TMA
    bool tima_write_during_reload{true}; // Writing TIMA during reload cycle overrides TMA value

    // APU Quirks
    bool apu_power_off_registers{true};// Clearing NR52 bit 7 resets registers (except Wave RAM on DMG)
    bool apu_length_counter_glitch{true};// Writing NRx4 when frame sequencer is at length step

    void set_preset(HardwareModel new_model) {
        model = new_model;
        if (model == HardwareModel::CGB_RevC || model == HardwareModel::AGB) {
            oam_corruption_bug = false;
            vblank_stat_bug = false;
            stat_interrupt_glitch = false;
        } else {
            oam_corruption_bug = true;
            vblank_stat_bug = true;
            stat_interrupt_glitch = true;
        }
    }
};

} // namespace gb
