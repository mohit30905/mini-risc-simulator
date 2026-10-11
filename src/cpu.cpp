// ============================================================================
// cpu.cpp — Mini RISC CPU Simulator Implementation
// ============================================================================
// Implements the classic fetch → decode → execute loop for a 16-bit RISC
// processor with 8 registers and 9 instructions.
// ============================================================================
#include "cpu.hpp"
#include "decoder.hpp"

#include <iostream>
#include <iomanip>

namespace mini_risc {

CPU::CPU() { reset(); }

void CPU::load(const std::vector<uint16_t>& program) {
    memory_ = program;
}

void CPU::reset() {
    for (int i = 0; i < NUM_REGISTERS; ++i) state_.regs[i] = 0;
    state_.pc     = 0;
    state_.halted = false;
    state_.steps  = 0;
    trace_.clear();
    trace_on_ = false;
    quiet_    = false;
}

uint16_t CPU::reg(int r) const {
    if (r < 0 || r >= NUM_REGISTERS) return 0;
    return state_.regs[r];
}

void CPU::set_reg(int r, uint16_t v) {
    if (r >= 0 && r < NUM_REGISTERS) state_.regs[r] = v;
}

// ════════════════════════════════════════════════════════════════════════════
// run — execute until HALT or step limit
// ════════════════════════════════════════════════════════════════════════════
bool CPU::run(bool trace) {
    trace_on_ = trace;
    while (!state_.halted && state_.steps < MAX_EXECUTION_STEPS) {
        if (!step()) break;
    }
    return state_.halted;
}

// ════════════════════════════════════════════════════════════════════════════
// step — fetch, decode, execute ONE instruction
// ════════════════════════════════════════════════════════════════════════════
bool CPU::step() {
    if (state_.halted) return false;

    // Fetch
    if (state_.pc >= memory_.size()) {
        if (!quiet_) {
            std::cerr << "Error: PC (" << state_.pc
                      << ") out of instruction memory bounds ("
                      << memory_.size() << ")\n";
        }
        state_.halted = true;
        return false;
    }
    uint16_t word = memory_[state_.pc];

    // Decode
    Instruction inst = Decoder::decode(word);
    if (!inst.valid) {
        if (!quiet_) {
            std::cerr << "Error: invalid instruction 0x"
                      << std::hex << std::setw(4) << std::setfill('0') << word
                      << " at PC=" << std::dec << state_.pc << "\n";
        }
        state_.halted = true;
        return false;
    }

    // Record trace entry (pre-execute snapshot)
    TraceEntry entry;
    entry.step         = state_.steps;
    entry.pc           = state_.pc;
    entry.machine_word = word;
    entry.disassembly  = Decoder::disassemble(word);
    entry.reg_changed  = -1;
    entry.old_value    = 0;
    entry.new_value    = 0;

    // Capture old register values for trace
    uint16_t old_regs[NUM_REGISTERS];
    for (int i = 0; i < NUM_REGISTERS; ++i) old_regs[i] = state_.regs[i];

    // Execute
    execute(inst);
    state_.steps++;

    // Determine which register changed (if any) for the trace
    if (inst.opcode != Opcode::HALT) {
        int rd = inst.rd;
        if (old_regs[rd] != state_.regs[rd]) {
            entry.reg_changed = rd;
            entry.old_value   = old_regs[rd];
            entry.new_value   = state_.regs[rd];
        } else {
            // Register written but value didn't change — still record it
            Format fmt = get_format(inst.opcode);
            if (fmt != Format::H_TYPE) {
                entry.reg_changed = rd;
                entry.old_value   = old_regs[rd];
                entry.new_value   = state_.regs[rd];
            }
        }
    }

    if (trace_on_) trace_.push_back(entry);

    return !state_.halted;
}

// ════════════════════════════════════════════════════════════════════════════
// execute — carry out a decoded instruction
// ════════════════════════════════════════════════════════════════════════════
void CPU::execute(const Instruction& inst) {
    switch (inst.opcode) {
        case Opcode::HALT:
            state_.halted = true;
            return;   // do NOT increment PC

        case Opcode::MOVI:
            // Store the sign-extended immediate into Rd.
            // Casting int16_t → uint16_t preserves the bit pattern.
            state_.regs[inst.rd] = static_cast<uint16_t>(inst.imm);
            break;

        case Opcode::ADD:
            state_.regs[inst.rd] =
                static_cast<uint16_t>(state_.regs[inst.rs1] +
                                      state_.regs[inst.rs2]);
            break;

        case Opcode::SUB:
            state_.regs[inst.rd] =
                static_cast<uint16_t>(state_.regs[inst.rs1] -
                                      state_.regs[inst.rs2]);
            break;

        case Opcode::AND:
            state_.regs[inst.rd] =
                state_.regs[inst.rs1] & state_.regs[inst.rs2];
            break;

        case Opcode::OR:
            state_.regs[inst.rd] =
                state_.regs[inst.rs1] | state_.regs[inst.rs2];
            break;

        case Opcode::XOR:
            state_.regs[inst.rd] =
                state_.regs[inst.rs1] ^ state_.regs[inst.rs2];
            break;

        case Opcode::SHL:
            state_.regs[inst.rd] =
                static_cast<uint16_t>(state_.regs[inst.rs1] << inst.shamt);
            break;

        case Opcode::SHR:
            state_.regs[inst.rd] =
                static_cast<uint16_t>(state_.regs[inst.rs1] >> inst.shamt);
            break;

        default:
            std::cerr << "Error: unhandled opcode in execute\n";
            state_.halted = true;
            return;
    }
    // Advance program counter (HALT already returned above)
    state_.pc++;
}

// ════════════════════════════════════════════════════════════════════════════
// print helpers
// ════════════════════════════════════════════════════════════════════════════
void CPU::print_registers() const {
    std::cout << "\n=== Register File ===\n";
    for (int i = 0; i < NUM_REGISTERS; ++i) {
        std::cout << "  R" << i << " = "
                  << std::setw(5) << state_.regs[i]
                  << "  (0x" << std::hex << std::setw(4) << std::setfill('0')
                  << state_.regs[i] << std::dec << std::setfill(' ') << ")\n";
    }
    std::cout << "  PC = " << state_.pc
              << "  |  Steps = " << state_.steps
              << "  |  Halted = " << (state_.halted ? "yes" : "no") << "\n";
}

void CPU::print_trace() const {
    std::cout << "\n=== Execution Trace ===\n";
    for (const auto& e : trace_) {
        std::cout << "  Step " << std::setw(3) << e.step
                  << "  PC=" << std::setw(3) << e.pc
                  << "  [0x" << std::hex << std::setw(4) << std::setfill('0')
                  << e.machine_word << std::dec << std::setfill(' ')
                  << "]  " << std::setw(20) << std::left << e.disassembly
                  << std::right;
        if (e.reg_changed >= 0) {
            std::cout << "  R" << e.reg_changed << ": "
                      << e.old_value << " -> " << e.new_value;
        }
        std::cout << "\n";
    }
}

} // namespace mini_risc
