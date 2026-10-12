// ============================================================================
// isa.hpp — Mini RISC ISA Definitions
// ============================================================================
// Central definitions for the Mini RISC architecture:
//   - 16-bit instruction word
//   - 8 general-purpose registers (R0–R7), all writable
//   - 3 instruction formats: R-type, I-type, S-type, plus HALT (H-type)
//   - 9 instructions: HALT, MOVI, ADD, SUB, AND, OR, XOR, SHL, SHR
// ============================================================================
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace mini_risc {

//  Architecture constants 
constexpr int NUM_REGISTERS = 8;
constexpr int WORD_BITS = 16;
constexpr uint16_t WORD_MASK = 0xFFFF;
constexpr int MAX_EXECUTION_STEPS = 10000;

// Immediate field: 9-bit two's-complement (range -256 … 255)
constexpr int IMM_BITS = 9;
constexpr int IMM_MIN = -256;
constexpr int IMM_MAX = 255;
constexpr uint16_t IMM_MASK = 0x01FF; // lower 9 bits
constexpr uint16_t IMM_SIGN = 0x0100; // bit 8 = sign bit

// Shift-amount field: 4 bits (range 0 … 15)
constexpr int SHAMT_BITS = 4;
constexpr int SHAMT_MAX = 15;

//── Opcodes (4-bit, values 0x0 – 0x8) 
// Encoding occupies bits [15:12] of every instruction word.
enum class Opcode : uint8_t {
  HALT = 0x0,
  MOVI = 0x1,
  ADD = 0x2,
  SUB = 0x3,
  AND = 0x4,
  OR = 0x5,
  XOR = 0x6,
  SHL = 0x7,
  SHR = 0x8,
  INVALID = 0xF // sentinel — never appears in valid machine code
};

// ── Instruction formats ─────────────────────────────────────────────────────
//
//  R-type  (ADD, SUB, AND, OR, XOR):
//    [15:12] opcode  [11:9] Rd  [8:6] Rs1  [5:3] Rs2  [2:0] unused(0)
//
//  I-type  (MOVI):
//    [15:12] opcode  [11:9] Rd  [8:0] imm9  (signed, two's-complement)
//
//  S-type  (SHL, SHR):
//    [15:12] opcode  [11:9] Rd  [8:6] Rs  [5:2] shamt  [1:0] unused(0)
//
//  H-type  (HALT):
//    [15:12] opcode  [11:0] unused(0)
//
enum class Format { R_TYPE, I_TYPE, S_TYPE, H_TYPE, UNKNOWN };

// ── Decoded instruction ─────────────────────────────────────────────────────
struct Instruction {
    Opcode   opcode = Opcode::INVALID;
    uint8_t  rd     = 0;      // destination register  (all formats except HALT)
    uint8_t  rs1    = 0;      // source register 1     (R-type)
    uint8_t  rs2    = 0;      // source register 2     (R-type)
    int16_t  imm    = 0;      // sign-extended 9-bit   (I-type)
    uint8_t  shamt  = 0;      // shift amount 0–15     (S-type)
    bool     valid  = false;
};

// ── Helper: opcode → format ─────────────────────────────────────────────────
inline Format get_format(Opcode op) {
  switch (op) {
  case Opcode::HALT:
    return Format::H_TYPE;
  case Opcode::MOVI:
    return Format::I_TYPE;
  case Opcode::ADD:
  case Opcode::SUB:
  case Opcode::AND:
  case Opcode::OR:
  case Opcode::XOR:
    return Format::R_TYPE;
  case Opcode::SHL:
  case Opcode::SHR:
    return Format::S_TYPE;
  default:
    return Format::UNKNOWN;
  }
}

// ── Helper: opcode → string ─────────────────────────────────────────────────
inline std::string opcode_to_string(Opcode op) {
  switch (op) {
  case Opcode::HALT:
    return "HALT";
  case Opcode::MOVI:
    return "MOVI";
  case Opcode::ADD:
    return "ADD";
  case Opcode::SUB:
    return "SUB";
  case Opcode::AND:
    return "AND";
  case Opcode::OR:
    return "OR";
  case Opcode::XOR:
    return "XOR";
  case Opcode::SHL:
    return "SHL";
  case Opcode::SHR:
    return "SHR";
  default:
    return "???";
  }
}

// ── Helper: string → opcode (case-insensitive) ─────────────────────────────
inline Opcode string_to_opcode(const std::string &s) {
  std::string upper = s;
  std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
  if (upper == "HALT")
    return Opcode::HALT;
  if (upper == "MOVI")
    return Opcode::MOVI;
  if (upper == "ADD")
    return Opcode::ADD;
  if (upper == "SUB")
    return Opcode::SUB;
  if (upper == "AND")
    return Opcode::AND;
  if (upper == "OR")
    return Opcode::OR;
  if (upper == "XOR")
    return Opcode::XOR;
  if (upper == "SHL")
    return Opcode::SHL;
  if (upper == "SHR")
    return Opcode::SHR;
  return Opcode::INVALID;
}

} // namespace mini_risc
