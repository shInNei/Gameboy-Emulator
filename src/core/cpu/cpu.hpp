#pragma once

#include "core/types.hpp"
#include "core/quirks.hpp"
#include "core/cpu/registers.hpp"
#include "core/bus/interrupt_controller.hpp"
#include "core/bus/mmu.hpp"
#include "core/timer/timer.hpp"
#include "core/ppu/ppu.hpp"
#include "core/apu/apu.hpp"
#include "core/serial/serial.hpp"

namespace gb {

class CPU {
public:
    CPU(Registers& registers,
        InterruptController& interrupts,
        MMU& mmu,
        Timer& timer,
        PPU& ppu,
        APU& apu,
        Serial& serial,
        const HardwareQuirks& quirks)
        : regs(registers), interrupts(interrupts), mmu(mmu),
          timer(timer), ppu(ppu), apu(apu), serial(serial), quirks(quirks) {}

    void reset();
    u8 step(); // Executes 1 instruction, ticks peripherals, returns M-cycles

    [[nodiscard]] bool is_double_speed() const { return (mmu.get_key1() & 0x80) != 0; }

    [[nodiscard]] bool is_halted() const { return halted; }
    [[nodiscard]] bool is_stopped() const { return stopped; }

    void request_halt() { halted = true; }
    void request_stop() { stopped = true; }

    [[nodiscard]] Cycles cycle_count() const { return total_cycles; }
    [[nodiscard]] const Registers& get_registers() const { return regs; }

private:
    u8 handle_interrupts();
    u8 execute_opcode(u8 opcode);
    u8 execute_cb_opcode(u8 opcode);

    // ALU Helpers
    void alu_add(u8 val);
    void alu_adc(u8 val);
    void alu_sub(u8 val);
    void alu_sbc(u8 val);
    void alu_and(u8 val);
    void alu_xor(u8 val);
    void alu_or(u8 val);
    void alu_cp(u8 val);
    u8 alu_inc(u8 val);
    u8 alu_dec(u8 val);
    u16 alu_add16(u16 base, u16 addend);
    u16 alu_add_sp_e8(i8 offset);

    u8 rlc(u8 val);
    u8 rrc(u8 val);
    u8 rl(u8 val);
    u8 rr(u8 val);
    u8 sla(u8 val);
    u8 sra(u8 val);
    u8 swap(u8 val);
    u8 srl(u8 val);
    void bit_test(u8 val, u8 bit_num);

    void push16(u16 val);
    u16 pop16();

    Registers& regs;
    InterruptController& interrupts;
    MMU& mmu;
    Timer& timer;
    PPU& ppu;
    APU& apu;
    Serial& serial;
    const HardwareQuirks& quirks;

    bool halted{false};
    bool stopped{false};
    bool halt_bug_triggered{false};
    Cycles total_cycles{0};
};

} // namespace gb
