// ============================================================================
// cpu.hpp — Mini RISC CPU Simulator Interface
// ============================================================================
#pragma once

#include "isa.hpp"
#include <vector>
#include <string>

namespace mini_risc {

// ── Snapshot of CPU state ───────────────────────────────────────────────────
struct CPUState {
    uint16_t regs[NUM_REGISTERS] = {};   // R0–R7, all initialised to 0
    uint16_t pc      = 0;
    bool     halted  = false;
    int      steps   = 0;
};

// ── One line of the execution trace ─────────────────────────────────────────
struct TraceEntry {
    int         step;
    uint16_t    pc;
    uint16_t    machine_word;
    std::string disassembly;
    int         reg_changed;     // register index, or -1 if none
    uint16_t    old_value;
    uint16_t    new_value;
};

// ── CPU ─────────────────────────────────────────────────────────────────────
class CPU {
public:
    CPU();

    // Load a program (vector of encoded words) into instruction memory.
    void load(const std::vector<uint16_t>& program);

    // Reset all registers, PC, and trace.
    void reset();

    // Run until HALT or step limit.  Returns true if halted normally.
    bool run(bool trace = false);

    // Execute exactly one instruction.  Returns true if can continue.
    bool step();

    // ── Accessors ───────────────────────────────────────────────────────────
    const CPUState&              state()  const { return state_; }
    uint16_t                     reg(int r) const;
    const std::vector<TraceEntry>& trace() const { return trace_; }

    // ── For unit tests ──────────────────────────────────────────────────────
    void set_reg(int r, uint16_t v);
    void set_quiet(bool q) { quiet_ = q; }
    bool is_quiet() const { return quiet_; }

    // Print final register dump
    void print_registers() const;
    // Print execution trace
    void print_trace() const;

private:
    CPUState                state_;
    std::vector<uint16_t>   memory_;     // instruction memory
    std::vector<TraceEntry> trace_;
    bool                    trace_on_ = false;
    bool                    quiet_    = false;

    void execute(const Instruction& inst);
};

} // namespace mini_risc
