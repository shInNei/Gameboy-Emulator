#pragma once

#include "core/system.hpp"
#include <string>
#include <fstream>
#include <format>

namespace gb {

class ExecutionTracer {
public:
    void enable(const std::string& log_filepath) {
        log_file.open(log_filepath, std::ios::out | std::ios::trunc);
        enabled = log_file.is_open();
    }

    void disable() {
        if (log_file.is_open()) log_file.close();
        enabled = false;
    }

    void trace(const System& sys) {
        if (!enabled || !log_file.is_open()) return;

        const auto& regs = sys.get_registers();
        const auto& mmu = const_cast<System&>(sys).get_mmu();

        u8 p1 = mmu.read(regs.pc);
        u8 p2 = mmu.read(regs.pc + 1);
        u8 p3 = mmu.read(regs.pc + 2);
        u8 p4 = mmu.read(regs.pc + 3);

        // Format according to Gameboy Doctor spec
        std::string line = std::format(
            "A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}\n",
            regs.a, regs.f, regs.b, regs.c, regs.d, regs.e, regs.h, regs.l, regs.sp, regs.pc, p1, p2, p3, p4
        );

        log_file << line;
    }

    [[nodiscard]] bool is_enabled() const { return enabled; }

private:
    std::ofstream log_file;
    bool enabled{false};
};

} // namespace gb
