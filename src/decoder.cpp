// ============================================================================
// decoder.cpp — Mini RISC Instruction Decoder Implementation
// ============================================================================
// Takes a 16-bit machine word and extracts opcode + operand fields according
// to the format determined by the opcode.
//
//  R-type  [15:12] op  [11:9] Rd  [8:6] Rs1  [5:3] Rs2  [2:0] 0
//  I-type  [15:12] op  [11:9] Rd  [8:0] imm9 (signed)
//  S-type  [15:12] op  [11:9] Rd  [8:6] Rs   [5:2] shamt [1:0] 0
//  H-type  [15:12] op  [11:0] 0
// ============================================================================
#include "decoder.hpp"
#include <sstream>

namespace mini_risc {

// ── Decode ──────────────────────────────────────────────────────────────────
Instruction Decoder::decode(uint16_t word) {
    Instruction inst;

    // Extract opcode from bits [15:12]
    uint8_t op_raw = static_cast<uint8_t>((word >> 12) & 0x0F);
    if (op_raw > static_cast<uint8_t>(Opcode::SHR)) {
        // Unknown opcode
        inst.valid = false;
        return inst;
    }
    inst.opcode = static_cast<Opcode>(op_raw);

    Format fmt = get_format(inst.opcode);

    switch (fmt) {
        case Format::H_TYPE:
            // Bits [11:0] must be zero for a valid HALT
            if ((word & 0x0FFF) != 0) {
                inst.valid = false;
                return inst;
            }
            inst.valid = true;
            break;

        case Format::I_TYPE:
            inst.rd = static_cast<uint8_t>((word >> 9) & 0x07);
            {
                uint16_t raw_imm = word & IMM_MASK;        // 9-bit value
                // Sign-extend from 9 bits to 16 bits
                if (raw_imm & IMM_SIGN) {
                    inst.imm = static_cast<int16_t>(raw_imm | 0xFE00);
                } else {
                    inst.imm = static_cast<int16_t>(raw_imm);
                }
            }
            inst.valid = true;
            break;

        case Format::R_TYPE:
            // Bits [2:0] must be zero for R-type
            if ((word & 0x0007) != 0) {
                inst.valid = false;
                return inst;
            }
            inst.rd  = static_cast<uint8_t>((word >> 9) & 0x07);
            inst.rs1 = static_cast<uint8_t>((word >> 6) & 0x07);
            inst.rs2 = static_cast<uint8_t>((word >> 3) & 0x07);
            inst.valid = true;
            break;

        case Format::S_TYPE:
            // Bits [1:0] must be zero for S-type
            if ((word & 0x0003) != 0) {
                inst.valid = false;
                return inst;
            }
            inst.rd    = static_cast<uint8_t>((word >> 9) & 0x07);
            inst.rs1   = static_cast<uint8_t>((word >> 6) & 0x07);
            inst.shamt = static_cast<uint8_t>((word >> 2) & 0x0F);
            inst.valid = true;
            break;

        default:
            inst.valid = false;
            break;
    }
    return inst;
}

// ── Disassemble ─────────────────────────────────────────────────────────────
std::string Decoder::disassemble(uint16_t word) {
    Instruction inst = decode(word);
    if (!inst.valid) return "??? (invalid)";

    std::ostringstream oss;
    oss << opcode_to_string(inst.opcode);

    Format fmt = get_format(inst.opcode);
    switch (fmt) {
        case Format::H_TYPE:
            break;
        case Format::I_TYPE:
            oss << " R" << static_cast<int>(inst.rd)
                << ", " << inst.imm;
            break;
        case Format::R_TYPE:
            oss << " R" << static_cast<int>(inst.rd)
                << ", R" << static_cast<int>(inst.rs1)
                << ", R" << static_cast<int>(inst.rs2);
            break;
        case Format::S_TYPE:
            oss << " R" << static_cast<int>(inst.rd)
                << ", R" << static_cast<int>(inst.rs1)
                << ", " << static_cast<int>(inst.shamt);
            break;
        default:
            oss << " ???";
            break;
    }
    return oss.str();
}

// ── Validity check ──────────────────────────────────────────────────────────
bool Decoder::is_valid(uint16_t word) {
    return decode(word).valid;
}

} // namespace mini_risc
