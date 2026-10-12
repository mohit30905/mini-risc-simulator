// ============================================================================
// verifier.cpp — Mini RISC Verification Framework Implementation
// ============================================================================
// Provides an *independent* reference model and a comprehensive test suite
// that exercises every instruction, every encoder/decoder path, arithmetic
// boundaries, invalid inputs, and full-program execution.
//
// KEY DESIGN RULE:  The reference model functions (ref::add, ref::encode_r, …)
// are implemented WITHOUT calling the main Assembler, Decoder, or CPU classes.
// This prevents a shared bug from silently passing both paths.
// ============================================================================
#include "verifier.hpp"
#include "assembler.hpp"
#include "decoder.hpp"
#include "cpu.hpp"

#include <iostream>
#include <sstream>

namespace mini_risc {

// ════════════════════════════════════════════════════════════════════════════
// Reference model — standalone implementations
// ════════════════════════════════════════════════════════════════════════════
namespace ref {

uint16_t add (uint16_t a, uint16_t b) { return static_cast<uint16_t>((a + b) & 0xFFFF); }
uint16_t sub (uint16_t a, uint16_t b) { return static_cast<uint16_t>((a - b) & 0xFFFF); }
uint16_t band(uint16_t a, uint16_t b) { return a & b; }
uint16_t bor (uint16_t a, uint16_t b) { return a | b; }
uint16_t bxor(uint16_t a, uint16_t b) { return a ^ b; }
uint16_t shl (uint16_t a, uint8_t amt) { return static_cast<uint16_t>((a << amt) & 0xFFFF); }
uint16_t shr (uint16_t a, uint8_t amt) { return static_cast<uint16_t>(a >> amt); }

// ── Independent encoder ────────────────────────────────────────────────────
// These construct machine words by manual bit manipulation — NO dependency
// on Assembler::encode().

uint16_t encode_r(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    uint16_t w = 0;
    w |= (static_cast<uint16_t>(opcode) & 0x0F) << 12;
    w |= (static_cast<uint16_t>(rd)     & 0x07) << 9;
    w |= (static_cast<uint16_t>(rs1)    & 0x07) << 6;
    w |= (static_cast<uint16_t>(rs2)    & 0x07) << 3;
    return w;
}

uint16_t encode_i(uint8_t opcode, uint8_t rd, int16_t imm) {
    uint16_t w = 0;
    w |= (static_cast<uint16_t>(opcode) & 0x0F) << 12;
    w |= (static_cast<uint16_t>(rd)     & 0x07) << 9;
    w |= static_cast<uint16_t>(imm) & 0x01FF;
    return w;
}

uint16_t encode_s(uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t shamt) {
    uint16_t w = 0;
    w |= (static_cast<uint16_t>(opcode) & 0x0F) << 12;
    w |= (static_cast<uint16_t>(rd)     & 0x07) << 9;
    w |= (static_cast<uint16_t>(rs)     & 0x07) << 6;
    w |= (static_cast<uint16_t>(shamt)  & 0x0F) << 2;
    return w;
}

uint16_t encode_h(uint8_t opcode) {
    return static_cast<uint16_t>((opcode & 0x0F)) << 12;
}

} // namespace ref

// ════════════════════════════════════════════════════════════════════════════
// Verifier helpers
// ════════════════════════════════════════════════════════════════════════════
void Verifier::check(const std::string& name, bool condition,
                     const std::string& fail_detail) {
    TestResult r;
    r.name   = name;
    r.passed = condition;
    r.detail = condition ? "" : fail_detail;
    results_.push_back(r);
    if (condition) ++pass_; else ++fail_;
}

// ════════════════════════════════════════════════════════════════════════════
// Test: encode → decode round trip
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_encode_decode_roundtrip() {
    // R-type: ADD R3, R1, R2
    {
        Instruction orig;
        orig.opcode = Opcode::ADD; orig.rd = 3; orig.rs1 = 1; orig.rs2 = 2;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip ADD R3,R1,R2: valid",   dec.valid);
        check("roundtrip ADD R3,R1,R2: opcode",  dec.opcode == Opcode::ADD);
        check("roundtrip ADD R3,R1,R2: rd",      dec.rd == 3);
        check("roundtrip ADD R3,R1,R2: rs1",     dec.rs1 == 1);
        check("roundtrip ADD R3,R1,R2: rs2",     dec.rs2 == 2);
    }
    // R-type: XOR R7, R0, R5
    {
        Instruction orig;
        orig.opcode = Opcode::XOR; orig.rd = 7; orig.rs1 = 0; orig.rs2 = 5;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip XOR R7,R0,R5: valid",   dec.valid);
        check("roundtrip XOR R7,R0,R5: opcode",  dec.opcode == Opcode::XOR);
        check("roundtrip XOR R7,R0,R5: rd=7",    dec.rd == 7);
        check("roundtrip XOR R7,R0,R5: rs1=0",   dec.rs1 == 0);
        check("roundtrip XOR R7,R0,R5: rs2=5",   dec.rs2 == 5);
    }
    // I-type: MOVI R5, 100
    {
        Instruction orig;
        orig.opcode = Opcode::MOVI; orig.rd = 5; orig.imm = 100;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip MOVI R5,100: valid",    dec.valid);
        check("roundtrip MOVI R5,100: opcode",   dec.opcode == Opcode::MOVI);
        check("roundtrip MOVI R5,100: rd=5",     dec.rd == 5);
        check("roundtrip MOVI R5,100: imm=100",  dec.imm == 100);
    }
    // I-type: MOVI R0, -1
    {
        Instruction orig;
        orig.opcode = Opcode::MOVI; orig.rd = 0; orig.imm = -1;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip MOVI R0,-1: valid",     dec.valid);
        check("roundtrip MOVI R0,-1: imm=-1",    dec.imm == -1);
    }
    // I-type: MOVI R2, -256 (minimum)
    {
        Instruction orig;
        orig.opcode = Opcode::MOVI; orig.rd = 2; orig.imm = -256;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip MOVI R2,-256: imm",     dec.imm == -256);
    }
    // I-type: MOVI R3, 255 (maximum)
    {
        Instruction orig;
        orig.opcode = Opcode::MOVI; orig.rd = 3; orig.imm = 255;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip MOVI R3,255: imm",      dec.imm == 255);
    }
    // S-type: SHL R2, R1, 4
    {
        Instruction orig;
        orig.opcode = Opcode::SHL; orig.rd = 2; orig.rs1 = 1; orig.shamt = 4;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip SHL R2,R1,4: valid",    dec.valid);
        check("roundtrip SHL R2,R1,4: opcode",   dec.opcode == Opcode::SHL);
        check("roundtrip SHL R2,R1,4: rd=2",     dec.rd == 2);
        check("roundtrip SHL R2,R1,4: rs1=1",    dec.rs1 == 1);
        check("roundtrip SHL R2,R1,4: shamt=4",  dec.shamt == 4);
    }
    // S-type: SHR R6, R7, 15
    {
        Instruction orig;
        orig.opcode = Opcode::SHR; orig.rd = 6; orig.rs1 = 7; orig.shamt = 15;
        orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip SHR R6,R7,15: shamt",   dec.shamt == 15);
    }
    // H-type: HALT
    {
        Instruction orig;
        orig.opcode = Opcode::HALT; orig.valid = true;
        uint16_t word = Assembler::encode(orig);
        Instruction dec = Decoder::decode(word);
        check("roundtrip HALT: valid",            dec.valid);
        check("roundtrip HALT: opcode",           dec.opcode == Opcode::HALT);
        check("roundtrip HALT: word==0",          word == 0x0000);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: reference encoder vs. main encoder
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_reference_vs_main_encoder() {
    // Compare ADD R3, R1, R2
    {
        Instruction inst;
        inst.opcode = Opcode::ADD; inst.rd = 3; inst.rs1 = 1; inst.rs2 = 2;
        inst.valid = true;
        uint16_t main_word = Assembler::encode(inst);
        uint16_t ref_word  = ref::encode_r(0x2, 3, 1, 2);
        std::ostringstream oss;
        oss << "main=0x" << std::hex << main_word
            << " ref=0x" << ref_word;
        check("ref_vs_main ADD R3,R1,R2",  main_word == ref_word, oss.str());
    }
    // Compare MOVI R5, 100
    {
        Instruction inst;
        inst.opcode = Opcode::MOVI; inst.rd = 5; inst.imm = 100;
        inst.valid = true;
        uint16_t main_word = Assembler::encode(inst);
        uint16_t ref_word  = ref::encode_i(0x1, 5, 100);
        std::ostringstream oss;
        oss << "main=0x" << std::hex << main_word
            << " ref=0x" << ref_word;
        check("ref_vs_main MOVI R5,100",   main_word == ref_word, oss.str());
    }
    // Compare MOVI R0, -1
    {
        Instruction inst;
        inst.opcode = Opcode::MOVI; inst.rd = 0; inst.imm = -1;
        inst.valid = true;
        uint16_t main_word = Assembler::encode(inst);
        uint16_t ref_word  = ref::encode_i(0x1, 0, -1);
        std::ostringstream oss;
        oss << "main=0x" << std::hex << main_word
            << " ref=0x" << ref_word;
        check("ref_vs_main MOVI R0,-1",    main_word == ref_word, oss.str());
    }
    // Compare SHL R2, R1, 4
    {
        Instruction inst;
        inst.opcode = Opcode::SHL; inst.rd = 2; inst.rs1 = 1; inst.shamt = 4;
        inst.valid = true;
        uint16_t main_word = Assembler::encode(inst);
        uint16_t ref_word  = ref::encode_s(0x7, 2, 1, 4);
        std::ostringstream oss;
        oss << "main=0x" << std::hex << main_word
            << " ref=0x" << ref_word;
        check("ref_vs_main SHL R2,R1,4",   main_word == ref_word, oss.str());
    }
    // Compare HALT
    {
        Instruction inst;
        inst.opcode = Opcode::HALT; inst.valid = true;
        uint16_t main_word = Assembler::encode(inst);
        uint16_t ref_word  = ref::encode_h(0x0);
        check("ref_vs_main HALT",          main_word == ref_word);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: arithmetic operations via CPU
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_arithmetic() {
    // Helper lambda: run one ALU test
    auto test_alu = [&](const std::string& label, Opcode op,
                        uint16_t a, uint16_t b, uint16_t expected) {
        CPU cpu;
        // Build a 3-instruction program: MOVI R1,a; MOVI R2,b; OP R3,R1,R2; HALT
        // But MOVI only handles 9-bit signed, so for large values we set regs directly
        std::vector<uint16_t> prog;
        Instruction inst;

        // Use direct register setup for arbitrary values
        cpu.reset();
        cpu.set_reg(1, a);
        cpu.set_reg(2, b);

        // Encode the ALU operation
        inst.opcode = op; inst.rd = 3; inst.rs1 = 1; inst.rs2 = 2;
        inst.valid = true;
        prog.push_back(Assembler::encode(inst));
        // HALT
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));

        cpu.load(prog);
        cpu.run();

        std::ostringstream oss;
        oss << "got=" << cpu.reg(3) << " expected=" << expected;
        check(label, cpu.reg(3) == expected, oss.str());
    };

    // ADD
    test_alu("ADD 10+20=30",         Opcode::ADD,  10,    20,    ref::add(10, 20));
    test_alu("ADD 0+0=0",           Opcode::ADD,  0,     0,     0);
    test_alu("ADD 0xFFFF+1=0",      Opcode::ADD,  0xFFFF, 1,    ref::add(0xFFFF, 1));
    test_alu("ADD 0x7FFF+1=0x8000", Opcode::ADD,  0x7FFF, 1,    ref::add(0x7FFF, 1));
    test_alu("ADD 100+200=300",     Opcode::ADD,  100,   200,   ref::add(100, 200));

    // SUB
    test_alu("SUB 20-10=10",        Opcode::SUB,  20,    10,    ref::sub(20, 10));
    test_alu("SUB 0-1=0xFFFF",      Opcode::SUB,  0,     1,     ref::sub(0, 1));
    test_alu("SUB 100-100=0",       Opcode::SUB,  100,   100,   0);
    test_alu("SUB 0xFFFF-0xFFFF=0", Opcode::SUB,  0xFFFF, 0xFFFF, 0);
}

// ════════════════════════════════════════════════════════════════════════════
// Test: bitwise operations
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_bitwise() {
    auto test_alu = [&](const std::string& label, Opcode op,
                        uint16_t a, uint16_t b, uint16_t expected) {
        CPU cpu;
        cpu.reset();
        cpu.set_reg(1, a);
        cpu.set_reg(2, b);

        Instruction inst;
        inst.opcode = op; inst.rd = 3; inst.rs1 = 1; inst.rs2 = 2;
        inst.valid = true;
        std::vector<uint16_t> prog;
        prog.push_back(Assembler::encode(inst));
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));

        cpu.load(prog);
        cpu.run();

        std::ostringstream oss;
        oss << "got=" << cpu.reg(3) << " expected=" << expected;
        check(label, cpu.reg(3) == expected, oss.str());
    };

    // AND
    test_alu("AND 0xFF&0x0F=0x0F",     Opcode::AND, 0x00FF, 0x000F, ref::band(0x00FF, 0x000F));
    test_alu("AND 0xFFFF&0=0",         Opcode::AND, 0xFFFF, 0x0000, 0);
    test_alu("AND 0xAAAA&0x5555=0",    Opcode::AND, 0xAAAA, 0x5555, 0);
    test_alu("AND 0xFFFF&0xFFFF",      Opcode::AND, 0xFFFF, 0xFFFF, 0xFFFF);

    // OR
    test_alu("OR 0xF0|0x0F=0xFF",      Opcode::OR,  0x00F0, 0x000F, ref::bor(0x00F0, 0x000F));
    test_alu("OR 0|0=0",              Opcode::OR,  0,      0,      0);
    test_alu("OR 0xAAAA|0x5555=0xFFFF",Opcode::OR,  0xAAAA, 0x5555, 0xFFFF);

    // XOR
    test_alu("XOR 10^20=30",          Opcode::XOR, 10,     20,     ref::bxor(10, 20));
    test_alu("XOR 0xFF^0xFF=0",        Opcode::XOR, 0x00FF, 0x00FF, 0);
    test_alu("XOR 0^0xFFFF=0xFFFF",    Opcode::XOR, 0,      0xFFFF, 0xFFFF);
    test_alu("XOR 0xAAAA^0x5555",      Opcode::XOR, 0xAAAA, 0x5555, ref::bxor(0xAAAA, 0x5555));
}

// ════════════════════════════════════════════════════════════════════════════
// Test: shift operations
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_shifts() {
    auto test_shift = [&](const std::string& label, Opcode op,
                          uint16_t val, uint8_t shamt, uint16_t expected) {
        CPU cpu;
        cpu.reset();
        cpu.set_reg(1, val);

        Instruction inst;
        inst.opcode = op; inst.rd = 2; inst.rs1 = 1; inst.shamt = shamt;
        inst.valid = true;
        std::vector<uint16_t> prog;
        prog.push_back(Assembler::encode(inst));
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));

        cpu.load(prog);
        cpu.run();

        std::ostringstream oss;
        oss << "got=" << cpu.reg(2) << " expected=" << expected;
        check(label, cpu.reg(2) == expected, oss.str());
    };

    // SHL
    test_shift("SHL 1<<0=1",          Opcode::SHL, 1,      0,  ref::shl(1, 0));
    test_shift("SHL 1<<1=2",          Opcode::SHL, 1,      1,  ref::shl(1, 1));
    test_shift("SHL 1<<15=0x8000",    Opcode::SHL, 1,      15, ref::shl(1, 15));
    test_shift("SHL 0xFF<<4=0x0FF0",  Opcode::SHL, 0x00FF, 4,  ref::shl(0x00FF, 4));
    test_shift("SHL 0x8000<<1=0",     Opcode::SHL, 0x8000, 1,  ref::shl(0x8000, 1));

    // SHR
    test_shift("SHR 0x8000>>15=1",    Opcode::SHR, 0x8000, 15, ref::shr(0x8000, 15));
    test_shift("SHR 0xFF>>4=0x0F",    Opcode::SHR, 0x00FF, 4,  ref::shr(0x00FF, 4));
    test_shift("SHR 1>>0=1",          Opcode::SHR, 1,      0,  ref::shr(1, 0));
    test_shift("SHR 1>>1=0",          Opcode::SHR, 1,      1,  0);
    test_shift("SHR 0xFFFF>>8=0xFF",  Opcode::SHR, 0xFFFF, 8,  ref::shr(0xFFFF, 8));
}

// ════════════════════════════════════════════════════════════════════════════
// Test: arithmetic boundary values
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_boundary_values() {
    // Overflow: 0xFFFF + 1 should wrap to 0
    {
        CPU cpu; cpu.reset();
        cpu.set_reg(1, 0xFFFF);
        cpu.set_reg(2, 1);
        Instruction inst;
        inst.opcode = Opcode::ADD; inst.rd = 3; inst.rs1 = 1; inst.rs2 = 2;
        inst.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(inst) };
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));
        cpu.load(prog); cpu.run();
        check("boundary ADD overflow wraps", cpu.reg(3) == 0,
              "got=" + std::to_string(cpu.reg(3)));
    }
    // Underflow: 0 - 1 should wrap to 0xFFFF
    {
        CPU cpu; cpu.reset();
        cpu.set_reg(1, 0);
        cpu.set_reg(2, 1);
        Instruction inst;
        inst.opcode = Opcode::SUB; inst.rd = 3; inst.rs1 = 1; inst.rs2 = 2;
        inst.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(inst) };
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));
        cpu.load(prog); cpu.run();
        check("boundary SUB underflow wraps", cpu.reg(3) == 0xFFFF,
              "got=" + std::to_string(cpu.reg(3)));
    }
    // All registers start at 0
    {
        CPU cpu; cpu.reset();
        bool all_zero = true;
        for (int i = 0; i < NUM_REGISTERS; ++i) {
            if (cpu.reg(i) != 0) { all_zero = false; break; }
        }
        check("boundary all regs init to 0", all_zero);
    }
    // R0 is writable
    {
        CPU cpu; cpu.reset();
        Instruction inst;
        inst.opcode = Opcode::MOVI; inst.rd = 0; inst.imm = 42;
        inst.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(inst) };
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));
        cpu.load(prog); cpu.run();
        check("boundary R0 is writable", cpu.reg(0) == 42,
              "got=" + std::to_string(cpu.reg(0)));
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: MOVI sign extension
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_movi_sign_extension() {
    // MOVI R1, -1  →  R1 = 0xFFFF
    {
        CPU cpu; cpu.reset();
        Instruction inst;
        inst.opcode = Opcode::MOVI; inst.rd = 1; inst.imm = -1;
        inst.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(inst) };
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));
        cpu.load(prog); cpu.run();
        check("MOVI -1 → 0xFFFF", cpu.reg(1) == 0xFFFF,
              "got=0x" + ([&]{ std::ostringstream o; o << std::hex << cpu.reg(1); return o.str(); })());
    }
    // MOVI R2, -256  →  R2 = 0xFF00
    {
        CPU cpu; cpu.reset();
        Instruction inst;
        inst.opcode = Opcode::MOVI; inst.rd = 2; inst.imm = -256;
        inst.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(inst) };
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));
        cpu.load(prog); cpu.run();
        check("MOVI -256 → 0xFF00", cpu.reg(2) == 0xFF00,
              "got=0x" + ([&]{ std::ostringstream o; o << std::hex << cpu.reg(2); return o.str(); })());
    }
    // MOVI R3, 255  →  R3 = 0x00FF
    {
        CPU cpu; cpu.reset();
        Instruction inst;
        inst.opcode = Opcode::MOVI; inst.rd = 3; inst.imm = 255;
        inst.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(inst) };
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));
        cpu.load(prog); cpu.run();
        check("MOVI 255 → 0x00FF", cpu.reg(3) == 0x00FF,
              "got=" + std::to_string(cpu.reg(3)));
    }
    // MOVI R4, 0  →  R4 = 0
    {
        CPU cpu; cpu.reset();
        cpu.set_reg(4, 9999);  // pre-load non-zero
        Instruction inst;
        inst.opcode = Opcode::MOVI; inst.rd = 4; inst.imm = 0;
        inst.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(inst) };
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        prog.push_back(Assembler::encode(halt));
        cpu.load(prog); cpu.run();
        check("MOVI 0 overwrites", cpu.reg(4) == 0,
              "got=" + std::to_string(cpu.reg(4)));
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: HALT and PC behaviour
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_halt_and_pc() {
    // HALT as first instruction — PC stays at 0, no more steps
    {
        CPU cpu; cpu.reset();
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(halt) };
        cpu.load(prog);
        cpu.run();
        check("HALT at PC=0: halted",   cpu.state().halted);
        check("HALT at PC=0: pc==0",    cpu.state().pc == 0);
        check("HALT at PC=0: steps==1", cpu.state().steps == 1);
    }
    // Three MOVIs then HALT — PC should be 3 (HALT doesn't increment)
    {
        CPU cpu; cpu.reset();
        Instruction m1; m1.opcode = Opcode::MOVI; m1.rd = 0; m1.imm = 1; m1.valid = true;
        Instruction m2; m2.opcode = Opcode::MOVI; m2.rd = 1; m2.imm = 2; m2.valid = true;
        Instruction m3; m3.opcode = Opcode::MOVI; m3.rd = 2; m3.imm = 3; m3.valid = true;
        Instruction halt; halt.opcode = Opcode::HALT; halt.valid = true;
        std::vector<uint16_t> prog = {
            Assembler::encode(m1), Assembler::encode(m2),
            Assembler::encode(m3), Assembler::encode(halt)
        };
        cpu.load(prog); cpu.run();
        check("3 MOVI+HALT: halted",    cpu.state().halted);
        check("3 MOVI+HALT: pc==3",     cpu.state().pc == 3);
        check("3 MOVI+HALT: steps==4",  cpu.state().steps == 4);
        check("3 MOVI+HALT: R0==1",     cpu.reg(0) == 1);
        check("3 MOVI+HALT: R1==2",     cpu.reg(1) == 2);
        check("3 MOVI+HALT: R2==3",     cpu.reg(2) == 3);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: execution step limit
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_execution_limit() {
    // 1. Program without HALT goes out-of-bounds cleanly
    {
        CPU cpu; cpu.reset();
        cpu.set_quiet(true);
        Instruction nop;
        nop.opcode = Opcode::ADD; nop.rd = 0; nop.rs1 = 0; nop.rs2 = 0;
        nop.valid = true;
        std::vector<uint16_t> prog = { Assembler::encode(nop) };
        cpu.load(prog);
        cpu.run();
        check("no HALT: cpu halted (OOB)", cpu.state().halted);
        check("no HALT: steps <= 2",       cpu.state().steps <= 2);
    }
    // 2. Program that exceeds MAX_EXECUTION_STEPS stops at the threshold
    {
        CPU cpu; cpu.reset();
        cpu.set_quiet(true);
        Instruction nop;
        nop.opcode = Opcode::ADD; nop.rd = 0; nop.rs1 = 0; nop.rs2 = 0;
        nop.valid = true;
        uint16_t nop_word = Assembler::encode(nop);
        std::vector<uint16_t> long_prog(MAX_EXECUTION_STEPS + 50, nop_word);
        cpu.load(long_prog);
        bool halted_cleanly = cpu.run();
        check("step limit: stopped at max steps", cpu.state().steps == MAX_EXECUTION_STEPS);
        check("step limit: did not halt normally", !halted_cleanly);
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: invalid assembly inputs
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_invalid_assembly() {
    Assembler as;
    std::vector<uint16_t> prog;

    // Unknown instruction
    {
        bool ok = as.assemble_lines({"FOO R1, R2"}, prog);
        check("invalid asm: unknown mnemonic", !ok);
    }
    // Wrong operand count
    {
        Assembler a2;
        bool ok = a2.assemble_lines({"ADD R1, R2"}, prog);
        check("invalid asm: ADD wrong operand count", !ok);
    }
    // Invalid register
    {
        Assembler a3;
        bool ok = a3.assemble_lines({"MOVI R9, 10"}, prog);
        check("invalid asm: R9 out of range", !ok);
    }
    // Immediate out of range
    {
        Assembler a4;
        bool ok = a4.assemble_lines({"MOVI R1, 300"}, prog);
        check("invalid asm: imm 300 > 255", !ok);
    }
    {
        Assembler a5;
        bool ok = a5.assemble_lines({"MOVI R1, -300"}, prog);
        check("invalid asm: imm -300 < -256", !ok);
    }
    // Shift amount out of range
    {
        Assembler a6;
        bool ok = a6.assemble_lines({"SHL R1, R2, 16"}, prog);
        check("invalid asm: shamt 16 > 15", !ok);
    }
    // HALT with operands
    {
        Assembler a7;
        bool ok = a7.assemble_lines({"HALT R1"}, prog);
        check("invalid asm: HALT with operand", !ok);
    }
    // Empty / comment-only lines are OK
    {
        Assembler a8;
        bool ok = a8.assemble_lines({"; comment", "  ", ""}, prog);
        check("valid asm: comment-only file", ok);
        check("valid asm: comment-only produces 0 instructions", prog.empty());
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: malformed machine code
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_malformed_machine_code() {
    // Opcode 0xF (15) is not a valid opcode — should decode as invalid
    {
        uint16_t bad = 0xF000;   // opcode=15, rest=0
        Instruction inst = Decoder::decode(bad);
        check("malformed: opcode 0xF invalid", !inst.valid);
    }
    // Opcode 0x9 (9) is also invalid
    {
        uint16_t bad = 0x9000;
        Instruction inst = Decoder::decode(bad);
        check("malformed: opcode 0x9 invalid", !inst.valid);
    }
    // Non-zero unused bits in HALT (0x0001) should decode as invalid
    {
        uint16_t bad = 0x0001;
        Instruction inst = Decoder::decode(bad);
        check("malformed: HALT non-zero unused bits", !inst.valid);
    }
    // Non-zero unused bits in R-type (0x2651 has bit 0 set)
    {
        uint16_t bad = 0x2651;
        Instruction inst = Decoder::decode(bad);
        check("malformed: R-type non-zero unused bits", !inst.valid);
    }
    // Non-zero unused bits in S-type (0x7451 has bit 0 set)
    {
        uint16_t bad = 0x7451;
        Instruction inst = Decoder::decode(bad);
        check("malformed: S-type non-zero unused bits", !inst.valid);
    }
    // Validity check function
    {
        check("is_valid(0x0000) = true",   Decoder::is_valid(0x0000));
        check("is_valid(0xF000) = false",  !Decoder::is_valid(0xF000));
        check("is_valid(0xA000) = false",  !Decoder::is_valid(0xA000));
        check("is_valid(0x0001) = false",  !Decoder::is_valid(0x0001));
        check("is_valid(0x2651) = false",  !Decoder::is_valid(0x2651));
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Test: full program — basic arithmetic
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_full_program_basic() {
    // The canonical test program from the spec:
    //   MOVI R1, 10
    //   MOVI R2, 20
    //   ADD  R3, R1, R2    →  R3 = 30
    //   SUB  R4, R2, R1    →  R4 = 10
    //   XOR  R5, R1, R2    →  R5 = 30  (10 XOR 20 = 30)
    //   HALT
    Assembler as;
    std::vector<uint16_t> prog;
    bool ok = as.assemble_lines({
        "MOVI R1, 10",
        "MOVI R2, 20",
        "ADD  R3, R1, R2",
        "SUB  R4, R2, R1",
        "XOR  R5, R1, R2",
        "HALT"
    }, prog);
    check("basic program: assembles", ok);
    check("basic program: 6 instructions", prog.size() == 6);

    CPU cpu;
    cpu.load(prog);
    cpu.run();

    check("basic program: halted",   cpu.state().halted);
    check("basic program: R1==10",   cpu.reg(1) == 10);
    check("basic program: R2==20",   cpu.reg(2) == 20);

    // Independent reference computation
    uint16_t exp_r3 = ref::add(10, 20);   // 30
    uint16_t exp_r4 = ref::sub(20, 10);   // 10
    uint16_t exp_r5 = ref::bxor(10, 20);  // 30

    std::ostringstream o3, o4, o5;
    o3 << "got=" << cpu.reg(3) << " expected=" << exp_r3;
    o4 << "got=" << cpu.reg(4) << " expected=" << exp_r4;
    o5 << "got=" << cpu.reg(5) << " expected=" << exp_r5;

    check("basic program: R3==30 (ADD)",   cpu.reg(3) == exp_r3, o3.str());
    check("basic program: R4==10 (SUB)",   cpu.reg(4) == exp_r4, o4.str());
    check("basic program: R5==30 (XOR)",   cpu.reg(5) == exp_r5, o5.str());
}

// ════════════════════════════════════════════════════════════════════════════
// Test: full program — bitwise
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_full_program_bitwise() {
    Assembler as;
    std::vector<uint16_t> prog;
    bool ok = as.assemble_lines({
        "MOVI R1, 0xFF",       // 255
        "MOVI R2, 0x0F",       // 15
        "AND  R3, R1, R2",     // 0x0F = 15
        "OR   R4, R1, R2",     // 0xFF = 255
        "XOR  R5, R1, R2",     // 0xF0 = 240
        "HALT"
    }, prog);
    check("bitwise program: assembles", ok);

    CPU cpu;
    cpu.load(prog);
    cpu.run();

    check("bitwise program: R3==15  (AND)",  cpu.reg(3) == ref::band(0xFF, 0x0F));
    check("bitwise program: R4==255 (OR)",   cpu.reg(4) == ref::bor(0xFF, 0x0F));
    check("bitwise program: R5==240 (XOR)",  cpu.reg(5) == ref::bxor(0xFF, 0x0F));
}

// ════════════════════════════════════════════════════════════════════════════
// Test: full program — shifts
// ════════════════════════════════════════════════════════════════════════════
void Verifier::test_full_program_shifts() {
    Assembler as;
    std::vector<uint16_t> prog;
    bool ok = as.assemble_lines({
        "MOVI R1, 1",
        "SHL  R2, R1, 4",     // 1 << 4 = 16
        "MOVI R3, 255",       // 0x00FF
        "SHR  R4, R3, 4",     // 0x00FF >> 4 = 0x000F = 15
        "HALT"
    }, prog);
    check("shift program: assembles", ok);

    CPU cpu;
    cpu.load(prog);
    cpu.run();

    check("shift program: R2==16  (SHL 1<<4)", cpu.reg(2) == ref::shl(1, 4));
    check("shift program: R4==15  (SHR 0xFF>>4)", cpu.reg(4) == ref::shr(0xFF, 4));
}

// ════════════════════════════════════════════════════════════════════════════
// run_all — orchestrate every test category
// ════════════════════════════════════════════════════════════════════════════
bool Verifier::run_all() {
    results_.clear();
    pass_ = fail_ = 0;

    test_encode_decode_roundtrip();
    test_reference_vs_main_encoder();
    test_arithmetic();
    test_bitwise();
    test_shifts();
    test_boundary_values();
    test_movi_sign_extension();
    test_halt_and_pc();
    test_execution_limit();
    test_invalid_assembly();
    test_malformed_machine_code();
    test_full_program_basic();
    test_full_program_bitwise();
    test_full_program_shifts();

    return fail_ == 0;
}

// ════════════════════════════════════════════════════════════════════════════
// print_report
// ════════════════════════════════════════════════════════════════════════════
void Verifier::print_report() const {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n"
              <<   "║         MINI RISC VERIFICATION REPORT           ║\n"
              <<   "╚══════════════════════════════════════════════════╝\n\n";

    for (const auto& r : results_) {
        std::cout << (r.passed ? "  [PASS] " : "  [FAIL] ") << r.name;
        if (!r.passed && !r.detail.empty()) {
            std::cout << "  — " << r.detail;
        }
        std::cout << "\n";
    }

    std::cout << "\n  ─────────────────────────────────────────────\n"
              << "  Total: " << (pass_ + fail_)
              << "  |  Passed: " << pass_
              << "  |  Failed: " << fail_ << "\n";

    if (fail_ == 0) {
        std::cout << "  ✓ ALL TESTS PASSED\n";
    } else {
        std::cout << "  ✗ SOME TESTS FAILED\n";
    }
    std::cout << "\n";
}

} // namespace mini_risc
