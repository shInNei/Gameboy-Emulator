#include "cpu.hpp"
#include <iostream>
#include "utils/bit_utils.hpp"

namespace gb {

void CPU::reset() {
    if (quirks.model == HardwareModel::CGB_RevC || quirks.model == HardwareModel::AGB) {
        regs.a = 0x11;
        regs.f = 0x80;
        regs.b = 0x00;
        regs.c = 0x00;
        regs.d = 0xFF;
        regs.e = 0x56;
        regs.h = 0x00;
        regs.l = 0x0D;
    } else {
        regs.a = 0x01;
        regs.f = 0xB0;
        regs.b = 0x00;
        regs.c = 0x13;
        regs.d = 0x00;
        regs.e = 0xD8;
        regs.h = 0x01;
        regs.l = 0x4D;
    }
    regs.sp = 0xFFFE;
    regs.pc = 0x0100;

    halted = false;
    stopped = false;
    halt_bug_triggered = false;
    total_cycles = 0;
}

void CPU::push16(u16 val) {
    regs.sp -= 2;
    if (regs.sp == 0xC204 || regs.sp + 1 == 0xC204 || regs.sp == 0xC301 || regs.sp + 1 == 0xC301) {
        std::cout << "PUSH16 TO " << std::hex << regs.sp << std::dec << " VAL: " << std::hex << val << " PC: " << std::hex << regs.pc << std::endl;
    }
    mmu.write16(regs.sp, val);
}

u16 CPU::pop16() {
    u16 val = mmu.read16(regs.sp);
    regs.sp += 2;
    return val;
}

u8 CPU::handle_interrupts() {
    if (!interrupts.ime() && !halted) return 0;
    if (!interrupts.is_pending()) return 0;

    // Exit HALT mode regardless of IME
    halted = false;

    if (!interrupts.ime()) return 0;

    u8 pending = interrupts.pending_mask();
    u8 irq_bit = 0;
    u16 vector = 0;

    if (bit::test(pending, 0)) { irq_bit = 0; vector = 0x0040; } // VBlank
    else if (bit::test(pending, 1)) { irq_bit = 1; vector = 0x0048; } // LCD STAT
    else if (bit::test(pending, 2)) { irq_bit = 2; vector = 0x0050; } // Timer
    else if (bit::test(pending, 3)) { irq_bit = 3; vector = 0x0058; } // Serial
    else if (bit::test(pending, 4)) { irq_bit = 4; vector = 0x0060; } // Joypad
    else return 0;

    interrupts.set_ime(false);
    interrupts.clear_interrupt(static_cast<InterruptType>(irq_bit));

    push16(regs.pc);
    regs.pc = vector;

    return 5; // 5 M-cycles (20 T-cycles)
}

u8 CPU::step() {
    u8 m_cycles = 0;
    
    u8 irq_cycles = handle_interrupts();
    if (irq_cycles > 0) {
        m_cycles = irq_cycles;
    } else if (halted) {
        m_cycles = 1;
    } else {
        // Fetch Opcode
        u8 opcode = mmu.read(regs.pc);
        if (halt_bug_triggered) {
            halt_bug_triggered = false; // PC does not advance on first fetch
        } else {
            regs.pc++;
        }
        m_cycles = execute_opcode(opcode);
    }

    u8 fast_t_cycles = m_cycles * 4;
    u8 sys_t_cycles = is_double_speed() ? (m_cycles * 2) : (m_cycles * 4);

    timer.tick(fast_t_cycles);
    mmu.tick_dma(fast_t_cycles);
    serial.tick(fast_t_cycles);

    ppu.tick(sys_t_cycles);
    apu.tick(sys_t_cycles);

    total_cycles += fast_t_cycles;
    interrupts.update_ei();

    return m_cycles;
}

// --- ALU HELPER IMPLEMENTATIONS ---
void CPU::alu_add(u8 val) {
    u16 res = regs.a + val;
    bool h = ((regs.a & 0x0F) + (val & 0x0F)) > 0x0F;
    bool c = res > 0xFF;
    regs.a = static_cast<u8>(res);
    regs.set_flags(regs.a == 0, false, h, c);
}

void CPU::alu_adc(u8 val) {
    u8 carry = regs.get_c() ? 1 : 0;
    u16 res = regs.a + val + carry;
    bool h = ((regs.a & 0x0F) + (val & 0x0F) + carry) > 0x0F;
    bool c = res > 0xFF;
    regs.a = static_cast<u8>(res);
    regs.set_flags(regs.a == 0, false, h, c);
}

void CPU::alu_sub(u8 val) {
    bool h = (regs.a & 0x0F) < (val & 0x0F);
    bool c = regs.a < val;
    regs.a -= val;
    regs.set_flags(regs.a == 0, true, h, c);
}

void CPU::alu_sbc(u8 val) {
    u8 carry = regs.get_c() ? 1 : 0;
    int res = regs.a - val - carry;
    bool h = (regs.a & 0x0F) < (val & 0x0F) + carry;
    bool c = res < 0;
    regs.a = static_cast<u8>(res);
    regs.set_flags(regs.a == 0, true, h, c);
}

void CPU::alu_and(u8 val) {
    regs.a &= val;
    regs.set_flags(regs.a == 0, false, true, false);
}

void CPU::alu_xor(u8 val) {
    regs.a ^= val;
    regs.set_flags(regs.a == 0, false, false, false);
}

void CPU::alu_or(u8 val) {
    regs.a |= val;
    regs.set_flags(regs.a == 0, false, false, false);
}

void CPU::alu_cp(u8 val) {
    bool h = (regs.a & 0x0F) < (val & 0x0F);
    bool c = regs.a < val;
    u8 res = regs.a - val;
    regs.set_flags(res == 0, true, h, c);
}

u8 CPU::alu_inc(u8 val) {
    bool h = (val & 0x0F) == 0x0F;
    u8 res = val + 1;
    regs.set_z(res == 0);
    regs.set_n(false);
    regs.set_h(h);
    return res;
}

u8 CPU::alu_dec(u8 val) {
    bool h = (val & 0x0F) == 0;
    u8 res = val - 1;
    regs.set_z(res == 0);
    regs.set_n(true);
    regs.set_h(h);
    return res;
}

u16 CPU::alu_add16(u16 base, u16 addend) {
    u32 res = base + addend;
    bool h = ((base & 0x0FFF) + (addend & 0x0FFF)) > 0x0FFF;
    bool c = res > 0xFFFF;
    regs.set_n(false);
    regs.set_h(h);
    regs.set_c(c);
    return static_cast<u16>(res);
}

u16 CPU::alu_add_sp_e8(i8 offset) {
    u16 sp = regs.sp;
    u8 uoffset = static_cast<u8>(offset);
    u16 res = sp + offset;
    bool h = ((sp & 0x0F) + (uoffset & 0x0F)) > 0x0F;
    bool c = ((sp & 0xFF) + uoffset) > 0xFF;
    regs.set_flags(false, false, h, c);
    return res;
}

u8 CPU::rlc(u8 val) {
    bool c = bit::test(val, 7);
    u8 res = (val << 1) | (c ? 1 : 0);
    regs.set_flags(res == 0, false, false, c);
    return res;
}

u8 CPU::rrc(u8 val) {
    bool c = bit::test(val, 0);
    u8 res = (val >> 1) | (c ? 0x80 : 0);
    regs.set_flags(res == 0, false, false, c);
    return res;
}

u8 CPU::rl(u8 val) {
    bool old_c = regs.get_c();
    bool c = bit::test(val, 7);
    u8 res = (val << 1) | (old_c ? 1 : 0);
    regs.set_flags(res == 0, false, false, c);
    return res;
}

u8 CPU::rr(u8 val) {
    bool old_c = regs.get_c();
    bool c = bit::test(val, 0);
    u8 res = (val >> 1) | (old_c ? 0x80 : 0);
    regs.set_flags(res == 0, false, false, c);
    return res;
}

u8 CPU::sla(u8 val) {
    bool c = bit::test(val, 7);
    u8 res = val << 1;
    regs.set_flags(res == 0, false, false, c);
    return res;
}

u8 CPU::sra(u8 val) {
    bool c = bit::test(val, 0);
    u8 res = (val >> 1) | (val & 0x80);
    regs.set_flags(res == 0, false, false, c);
    return res;
}

u8 CPU::swap(u8 val) {
    u8 res = (val >> 4) | (val << 4);
    regs.set_flags(res == 0, false, false, false);
    return res;
}

u8 CPU::srl(u8 val) {
    bool c = bit::test(val, 0);
    u8 res = val >> 1;
    regs.set_flags(res == 0, false, false, c);
    return res;
}

void CPU::bit_test(u8 val, u8 bit_num) {
    bool zero = !bit::test(val, bit_num);
    regs.set_z(zero);
    regs.set_n(false);
    regs.set_h(true);
}

// --- CB PREFIX OPCODES DECODER ---
u8 CPU::execute_cb_opcode(u8 cb_op) {
    u8 reg_idx = cb_op & 0x07;
    u8 group = (cb_op >> 6) & 0x03;
    u8 bit_num = (cb_op >> 3) & 0x07;

    auto get_reg = [&](u8 idx) -> u8 {
        switch (idx) {
            case 0: return regs.b; case 1: return regs.c;
            case 2: return regs.d; case 3: return regs.e;
            case 4: return regs.h; case 5: return regs.l;
            case 6: return mmu.read(regs.hl());
            case 7: return regs.a;
        }
        return 0;
    };

    auto set_reg = [&](u8 idx, u8 val) {
        switch (idx) {
            case 0: regs.b = val; break; case 1: regs.c = val; break;
            case 2: regs.d = val; break; case 3: regs.e = val; break;
            case 4: regs.h = val; break; case 5: regs.l = val; break;
            case 6: mmu.write(regs.hl(), val); break;
            case 7: regs.a = val; break;
        }
    };

    u8 val = get_reg(reg_idx);

    if (group == 0) { // Rotate / Shift
        switch (bit_num) {
            case 0: val = rlc(val); break;
            case 1: val = rrc(val); break;
            case 2: val = rl(val); break;
            case 3: val = rr(val); break;
            case 4: val = sla(val); break;
            case 5: val = sra(val); break;
            case 6: val = swap(val); break;
            case 7: val = srl(val); break;
        }
        set_reg(reg_idx, val);
    } else if (group == 1) { // BIT
        bit_test(val, bit_num);
    } else if (group == 2) { // RES
        bit::clear(val, bit_num);
        set_reg(reg_idx, val);
    } else if (group == 3) { // SET
        bit::set(val, bit_num);
        set_reg(reg_idx, val);
    }

    return (reg_idx == 6) ? ((group == 1) ? 3 : 4) : 2;
}

// --- MAIN OPCODE DISPATCHER ---
u8 CPU::execute_opcode(u8 op) {
    switch (op) {
        case 0x00: return 1; // NOP
        case 0x01: regs.bc(mmu.read16(regs.pc)); regs.pc += 2; return 3; // LD BC, n16
        case 0x02: mmu.write(regs.bc(), regs.a); return 2; // LD (BC), A
        case 0x03: regs.bc(regs.bc() + 1); return 2; // INC BC
        case 0x04: regs.b = alu_inc(regs.b); return 1; // INC B
        case 0x05: regs.b = alu_dec(regs.b); return 1; // DEC B
        case 0x06: regs.b = mmu.read(regs.pc++); return 2; // LD B, n8
        case 0x07: regs.a = rlc(regs.a); regs.set_z(false); return 1; // RLCA
        case 0x08: mmu.write16(mmu.read16(regs.pc), regs.sp); regs.pc += 2; return 5; // LD (a16), SP
        case 0x09: regs.hl(alu_add16(regs.hl(), regs.bc())); return 2; // ADD HL, BC
        case 0x0A: regs.a = mmu.read(regs.bc()); return 2; // LD A, (BC)
        case 0x0B: regs.bc(regs.bc() - 1); return 2; // DEC BC
        case 0x0C: regs.c = alu_inc(regs.c); return 1; // INC C
        case 0x0D: regs.c = alu_dec(regs.c); return 1; // DEC C
        case 0x0E: regs.c = mmu.read(regs.pc++); return 2; // LD C, n8
        case 0x0F: regs.a = rrc(regs.a); regs.set_z(false); return 1; // RRCA

        case 0x10: {
            regs.pc++;
            u8 key1 = mmu.get_key1();
            if (key1 & 0x01) {
                // Toggle bit 7 and clear bit 0
                mmu.set_key1((key1 ^ 0x80) & ~0x01);
            } else {
                stopped = true;
            }
            return 1;
        }
        case 0x11: regs.de(mmu.read16(regs.pc)); regs.pc += 2; return 3; // LD DE, n16
        case 0x12: mmu.write(regs.de(), regs.a); return 2; // LD (DE), A
        case 0x13: regs.de(regs.de() + 1); return 2; // INC DE
        case 0x14: regs.d = alu_inc(regs.d); return 1; // INC D
        case 0x15: regs.d = alu_dec(regs.d); return 1; // DEC D
        case 0x16: regs.d = mmu.read(regs.pc++); return 2; // LD D, n8
        case 0x17: regs.a = rl(regs.a); regs.set_z(false); return 1; // RLA
        case 0x18: { i8 offset = static_cast<i8>(mmu.read(regs.pc++)); regs.pc += offset; return 3; } // JR e8
        case 0x19: regs.hl(alu_add16(regs.hl(), regs.de())); return 2; // ADD HL, DE
        case 0x1A: regs.a = mmu.read(regs.de()); return 2; // LD A, (DE)
        case 0x1B: regs.de(regs.de() - 1); return 2; // DEC DE
        case 0x1C: regs.e = alu_inc(regs.e); return 1; // INC E
        case 0x1D: regs.e = alu_dec(regs.e); return 1; // DEC E
        case 0x1E: regs.e = mmu.read(regs.pc++); return 2; // LD E, n8
        case 0x1F: regs.a = rr(regs.a); regs.set_z(false); return 1; // RRA

        case 0x20: { // JR NZ, e8
            i8 offset = static_cast<i8>(mmu.read(regs.pc++));
            if (!regs.get_z()) { regs.pc += offset; return 3; }
            return 2;
        }
        case 0x21: regs.hl(mmu.read16(regs.pc)); regs.pc += 2; return 3; // LD HL, n16
        case 0x22: mmu.write(regs.hl(), regs.a); regs.hl(regs.hl() + 1); return 2; // LD (HL+), A
        case 0x23: regs.hl(regs.hl() + 1); return 2; // INC HL
        case 0x24: regs.h = alu_inc(regs.h); return 1; // INC H
        case 0x25: regs.h = alu_dec(regs.h); return 1; // DEC H
        case 0x26: regs.h = mmu.read(regs.pc++); return 2; // LD H, n8
        case 0x27: { // DAA (Decimal Adjust A for BCD)
            u8 correction = 0;
            bool set_c = false;
            if (regs.get_h() || (!regs.get_n() && (regs.a & 0x0F) > 0x09)) correction |= 0x06;
            if (regs.get_c() || (!regs.get_n() && regs.a > 0x99)) { correction |= 0x60; set_c = true; }
            regs.a += regs.get_n() ? -correction : correction;
            regs.set_z(regs.a == 0);
            regs.set_h(false);
            regs.set_c(set_c);
            return 1;
        }
        case 0x28: { // JR Z, e8
            i8 offset = static_cast<i8>(mmu.read(regs.pc++));
            if (regs.get_z()) { regs.pc += offset; return 3; }
            return 2;
        }
        case 0x29: regs.hl(alu_add16(regs.hl(), regs.hl())); return 2; // ADD HL, HL
        case 0x2A: regs.a = mmu.read(regs.hl()); regs.hl(regs.hl() + 1); return 2; // LD A, (HL+)
        case 0x2B: regs.hl(regs.hl() - 1); return 2; // DEC HL
        case 0x2C: regs.l = alu_inc(regs.l); return 1; // INC L
        case 0x2D: regs.l = alu_dec(regs.l); return 1; // DEC L
        case 0x2E: regs.l = mmu.read(regs.pc++); return 2; // LD L, n8
        case 0x2F: regs.a = ~regs.a; regs.set_n(true); regs.set_h(true); return 1; // CPL

        case 0x30: { // JR NC, e8
            i8 offset = static_cast<i8>(mmu.read(regs.pc++));
            if (!regs.get_c()) { regs.pc += offset; return 3; }
            return 2;
        }
        case 0x31: regs.sp = mmu.read16(regs.pc); regs.pc += 2; return 3; // LD SP, n16
        case 0x32: mmu.write(regs.hl(), regs.a); regs.hl(regs.hl() - 1); return 2; // LD (HL-), A
        case 0x33: regs.sp++; return 2; // INC SP
        case 0x34: mmu.write(regs.hl(), alu_inc(mmu.read(regs.hl()))); return 3; // INC (HL)
        case 0x35: mmu.write(regs.hl(), alu_dec(mmu.read(regs.hl()))); return 3; // DEC (HL)
        case 0x36: mmu.write(regs.hl(), mmu.read(regs.pc++)); return 3; // LD (HL), n8
        case 0x37: regs.set_n(false); regs.set_h(false); regs.set_c(true); return 1; // SCF
        case 0x38: { // JR C, e8
            i8 offset = static_cast<i8>(mmu.read(regs.pc++));
            if (regs.get_c()) { regs.pc += offset; return 3; }
            return 2;
        }
        case 0x39: regs.hl(alu_add16(regs.hl(), regs.sp)); return 2; // ADD HL, SP
        case 0x3A: regs.a = mmu.read(regs.hl()); regs.hl(regs.hl() - 1); return 2; // LD A, (HL-)
        case 0x3B: regs.sp--; return 2; // DEC SP
        case 0x3C: regs.a = alu_inc(regs.a); return 1; // INC A
        case 0x3D: regs.a = alu_dec(regs.a); return 1; // DEC A
        case 0x3E: regs.a = mmu.read(regs.pc++); return 2; // LD A, n8
        case 0x3F: regs.set_n(false); regs.set_h(false); regs.set_c(!regs.get_c()); return 1; // CCF

        // 0x40-0x7F LD r8, r8 & HALT
        case 0x76:
            if (quirks.halt_bug && !interrupts.ime() && interrupts.is_pending()) {
                halt_bug_triggered = true; // LR35902 HALT bug trigger
            } else {
                halted = true;
            }
            return 1;

        case 0xCB: return execute_cb_opcode(mmu.read(regs.pc++));

        // ALU A, r8
        case 0x80: alu_add(regs.b); return 1; case 0x81: alu_add(regs.c); return 1;
        case 0x82: alu_add(regs.d); return 1; case 0x83: alu_add(regs.e); return 1;
        case 0x84: alu_add(regs.h); return 1; case 0x85: alu_add(regs.l); return 1;
        case 0x86: alu_add(mmu.read(regs.hl())); return 2; case 0x87: alu_add(regs.a); return 1;
        
        case 0x88: alu_adc(regs.b); return 1; case 0x89: alu_adc(regs.c); return 1;
        case 0x8A: alu_adc(regs.d); return 1; case 0x8B: alu_adc(regs.e); return 1;
        case 0x8C: alu_adc(regs.h); return 1; case 0x8D: alu_adc(regs.l); return 1;
        case 0x8E: alu_adc(mmu.read(regs.hl())); return 2; case 0x8F: alu_adc(regs.a); return 1;

        case 0x90: alu_sub(regs.b); return 1; case 0x91: alu_sub(regs.c); return 1;
        case 0x92: alu_sub(regs.d); return 1; case 0x93: alu_sub(regs.e); return 1;
        case 0x94: alu_sub(regs.h); return 1; case 0x95: alu_sub(regs.l); return 1;
        case 0x96: alu_sub(mmu.read(regs.hl())); return 2; case 0x97: alu_sub(regs.a); return 1;

        case 0x98: alu_sbc(regs.b); return 1; case 0x99: alu_sbc(regs.c); return 1;
        case 0x9A: alu_sbc(regs.d); return 1; case 0x9B: alu_sbc(regs.e); return 1;
        case 0x9C: alu_sbc(regs.h); return 1; case 0x9D: alu_sbc(regs.l); return 1;
        case 0x9E: alu_sbc(mmu.read(regs.hl())); return 2; case 0x9F: alu_sbc(regs.a); return 1;

        case 0xA0: alu_and(regs.b); return 1; case 0xA1: alu_and(regs.c); return 1;
        case 0xA2: alu_and(regs.d); return 1; case 0xA3: alu_and(regs.e); return 1;
        case 0xA4: alu_and(regs.h); return 1; case 0xA5: alu_and(regs.l); return 1;
        case 0xA6: alu_and(mmu.read(regs.hl())); return 2; case 0xA7: alu_and(regs.a); return 1;

        case 0xA8: alu_xor(regs.b); return 1; case 0xA9: alu_xor(regs.c); return 1;
        case 0xAA: alu_xor(regs.d); return 1; case 0xAB: alu_xor(regs.e); return 1;
        case 0xAC: alu_xor(regs.h); return 1; case 0xAD: alu_xor(regs.l); return 1;
        case 0xAE: alu_xor(mmu.read(regs.hl())); return 2; case 0xAF: alu_xor(regs.a); return 1;

        case 0xB0: alu_or(regs.b); return 1; case 0xB1: alu_or(regs.c); return 1;
        case 0xB2: alu_or(regs.d); return 1; case 0xB3: alu_or(regs.e); return 1;
        case 0xB4: alu_or(regs.h); return 1; case 0xB5: alu_or(regs.l); return 1;
        case 0xB6: alu_or(mmu.read(regs.hl())); return 2; case 0xB7: alu_or(regs.a); return 1;

        case 0xB8: alu_cp(regs.b); return 1; case 0xB9: alu_cp(regs.c); return 1;
        case 0xBA: alu_cp(regs.d); return 1; case 0xBB: alu_cp(regs.e); return 1;
        case 0xBC: alu_cp(regs.h); return 1; case 0xBD: alu_cp(regs.l); return 1;
        case 0xBE: alu_cp(mmu.read(regs.hl())); return 2; case 0xBF: alu_cp(regs.a); return 1;

        // Returns & Jumps & Calls
        case 0xC0: if (!regs.get_z()) { regs.pc = pop16(); return 5; } return 2; // RET NZ
        case 0xC1: regs.bc(pop16()); return 3; // POP BC
        case 0xC2: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (!regs.get_z()) { regs.pc = addr; return 4; } return 3; } // JP NZ, a16
        case 0xC3: regs.pc = mmu.read16(regs.pc); return 4; // JP a16
        case 0xC4: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (!regs.get_z()) { push16(regs.pc); regs.pc = addr; return 6; } return 3; } // CALL NZ, a16
        case 0xC5: push16(regs.bc()); return 4; // PUSH BC
        case 0xC6: alu_add(mmu.read(regs.pc++)); return 2; // ADD A, n8
        case 0xC7: push16(regs.pc); regs.pc = 0x0000; return 4; // RST 00H
        case 0xC8: if (regs.get_z()) { regs.pc = pop16(); return 5; } return 2; // RET Z
        case 0xC9: regs.pc = pop16(); return 4; // RET
        case 0xCA: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (regs.get_z()) { regs.pc = addr; return 4; } return 3; } // JP Z, a16
        case 0xCC: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (regs.get_z()) { push16(regs.pc); regs.pc = addr; return 6; } return 3; } // CALL Z, a16
        case 0xCD: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; push16(regs.pc); regs.pc = addr; return 6; } // CALL a16
        case 0xCE: alu_adc(mmu.read(regs.pc++)); return 2; // ADC A, n8
        case 0xCF: push16(regs.pc); regs.pc = 0x0008; return 4; // RST 08H

        case 0xD0: if (!regs.get_c()) { regs.pc = pop16(); return 5; } return 2; // RET NC
        case 0xD1: regs.de(pop16()); return 3; // POP DE
        case 0xD2: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (!regs.get_c()) { regs.pc = addr; return 4; } return 3; } // JP NC, a16
        case 0xD4: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (!regs.get_c()) { push16(regs.pc); regs.pc = addr; return 6; } return 3; } // CALL NC, a16
        case 0xD5: push16(regs.de()); return 4; // PUSH DE
        case 0xD6: alu_sub(mmu.read(regs.pc++)); return 2; // SUB A, n8
        case 0xD7: push16(regs.pc); regs.pc = 0x0010; return 4; // RST 10H
        case 0xD8: if (regs.get_c()) { regs.pc = pop16(); return 5; } return 2; // RET C
        case 0xD9: regs.pc = pop16(); interrupts.set_ime(true); return 4; // RETI
        case 0xDA: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (regs.get_c()) { regs.pc = addr; return 4; } return 3; } // JP C, a16
        case 0xDC: { u16 addr = mmu.read16(regs.pc); regs.pc += 2; if (regs.get_c()) { push16(regs.pc); regs.pc = addr; return 6; } return 3; } // CALL C, a16
        case 0xDE: alu_sbc(mmu.read(regs.pc++)); return 2; // SBC A, n8
        case 0xDF: push16(regs.pc); regs.pc = 0x0018; return 4; // RST 18H

        case 0xE0: mmu.write(0xFF00 + mmu.read(regs.pc++), regs.a); return 3; // LDH (a8), A
        case 0xE1: regs.hl(pop16()); return 3; // POP HL
        case 0xE2: mmu.write(0xFF00 + regs.c, regs.a); return 2; // LD (C), A
        case 0xE5: push16(regs.hl()); return 4; // PUSH HL
        case 0xE6: alu_and(mmu.read(regs.pc++)); return 2; // AND A, n8
        case 0xE7: push16(regs.pc); regs.pc = 0x0020; return 4; // RST 20H
        case 0xE8: { i8 offset = static_cast<i8>(mmu.read(regs.pc++)); regs.sp = alu_add_sp_e8(offset); return 4; } // ADD SP, e8
        case 0xE9: regs.pc = regs.hl(); return 1; // JP HL
        case 0xEA: mmu.write(mmu.read16(regs.pc), regs.a); regs.pc += 2; return 4; // LD (a16), A
        case 0xEE: alu_xor(mmu.read(regs.pc++)); return 2; // XOR A, n8
        case 0xEF: push16(regs.pc); regs.pc = 0x0028; return 4; // RST 28H

        case 0xF0: regs.a = mmu.read(0xFF00 + mmu.read(regs.pc++)); return 3; // LDH A, (a8)
        case 0xF1: regs.af(pop16()); return 3; // POP AF
        case 0xF2: regs.a = mmu.read(0xFF00 + regs.c); return 2; // LD A, (C)
        case 0xF3: interrupts.set_ime(false); return 1; // DI
        case 0xF5: push16(regs.af()); return 4; // PUSH AF
        case 0xF6: alu_or(mmu.read(regs.pc++)); return 2; // OR A, n8
        case 0xF7: push16(regs.pc); regs.pc = 0x0030; return 4; // RST 30H
        case 0xF8: { i8 offset = static_cast<i8>(mmu.read(regs.pc++)); regs.hl(alu_add_sp_e8(offset)); return 3; } // LD HL, SP+e8
        case 0xF9: regs.sp = regs.hl(); return 2; // LD SP, HL
        case 0xFA: regs.a = mmu.read(mmu.read16(regs.pc)); regs.pc += 2; return 4; // LD A, (a16)
        case 0xFB: interrupts.schedule_ei(); return 1; // EI
        case 0xFE: alu_cp(mmu.read(regs.pc++)); return 2; // CP A, n8
        case 0xFF: push16(regs.pc); regs.pc = 0x0038; return 4; // RST 38H

        default:
            // Register-to-Register LD instructions (0x40 - 0x7F)
            if (op >= 0x40 && op <= 0x7F) {
                u8 src_idx = op & 0x07;
                u8 dst_idx = (op >> 3) & 0x07;

                auto read_val = [&](u8 idx) -> u8 {
                    switch (idx) {
                        case 0: return regs.b; case 1: return regs.c;
                        case 2: return regs.d; case 3: return regs.e;
                        case 4: return regs.h; case 5: return regs.l;
                        case 6: return mmu.read(regs.hl());
                        case 7: return regs.a;
                    }
                    return 0;
                };

                u8 val = read_val(src_idx);

                switch (dst_idx) {
                    case 0: regs.b = val; break; case 1: regs.c = val; break;
                    case 2: regs.d = val; break; case 3: regs.e = val; break;
                    case 4: regs.h = val; break; case 5: regs.l = val; break;
                    case 6: mmu.write(regs.hl(), val); break;
                    case 7: regs.a = val; break;
                }

                return (src_idx == 6 || dst_idx == 6) ? 2 : 1;
            }
            return 1;
    }
}

} // namespace gb
