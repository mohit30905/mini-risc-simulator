// ============================================================================
// verifier.hpp — Mini RISC Verification Framework Interface
// ============================================================================
#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace mini_risc {

// ── Single test result ──────────────────────────────────────────────────────
struct TestResult {
    std::string name;
    bool        passed;
    std::string detail;    // empty on pass; description of mismatch on fail
};

// ── Reference model ─────────────────────────────────────────────────────────
// Intentionally separate from CPU / Decoder / Assembler so that a shared bug
// in the main pipeline cannot silently pass verification.
namespace ref {
    uint16_t add (uint16_t a, uint16_t b);
    uint16_t sub (uint16_t a, uint16_t b);
    uint16_t band(uint16_t a, uint16_t b);
    uint16_t bor (uint16_t a, uint16_t b);
    uint16_t bxor(uint16_t a, uint16_t b);
    uint16_t shl (uint16_t a, uint8_t  amt);
    uint16_t shr (uint16_t a, uint8_t  amt);

    // Independent encoder (does NOT call Assembler::encode)
    uint16_t encode_r(uint8_t opcode, uint8_t rd, uint8_t rs1, uint8_t rs2);
    uint16_t encode_i(uint8_t opcode, uint8_t rd, int16_t imm);
    uint16_t encode_s(uint8_t opcode, uint8_t rd, uint8_t rs, uint8_t shamt);
    uint16_t encode_h(uint8_t opcode);
}

// ── Verifier ────────────────────────────────────────────────────────────────
class Verifier {
public:
    // Run the full test suite.  Returns true if all tests pass.
    bool run_all();

    // Print a summary report.
    void print_report() const;

    // Access individual results.
    const std::vector<TestResult>& results() const { return results_; }
    int pass_count() const { return pass_; }
    int fail_count() const { return fail_; }

private:
    std::vector<TestResult> results_;
    int pass_ = 0;
    int fail_ = 0;

    // Record a test outcome
    void check(const std::string& name, bool condition,
               const std::string& fail_detail = "");

    // ── Test categories ─────────────────────────────────────────────────────
    void test_encode_decode_roundtrip();
    void test_reference_vs_main_encoder();
    void test_arithmetic();
    void test_bitwise();
    void test_shifts();
    void test_boundary_values();
    void test_movi_sign_extension();
    void test_halt_and_pc();
    void test_execution_limit();
    void test_invalid_assembly();
    void test_malformed_machine_code();
    void test_full_program_basic();
    void test_full_program_bitwise();
    void test_full_program_shifts();
};

} // namespace mini_risc
