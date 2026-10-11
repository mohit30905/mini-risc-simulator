// ============================================================================
// decoder.hpp — Mini RISC Instruction Decoder Interface
// ============================================================================
#pragma once

#include "isa.hpp"
#include <string>

namespace mini_risc {

class Decoder {
public:
    // Decode a 16-bit machine word into an Instruction struct.
    // Sets inst.valid = false for unsupported or malformed encodings.
    static Instruction decode(uint16_t word);

    // Produce a human-readable disassembly string (e.g. "ADD R3, R1, R2").
    static std::string disassemble(uint16_t word);

    // Quick validity check (opcode in range, unused bits zero).
    static bool is_valid(uint16_t word);
};

} // namespace mini_risc
