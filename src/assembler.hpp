// ============================================================================
// assembler.hpp — Mini RISC Assembler Interface
// ============================================================================
#pragma once

#include "isa.hpp"
#include <vector>
#include <string>

namespace mini_risc {

// ── Assembler error record ──────────────────────────────────────────────────
struct AsmError {
    int         line;       // 1-based line number in the source file
    std::string message;
};

// ── Assembler ───────────────────────────────────────────────────────────────
// Reads .asm text, validates every field, and produces encoded 16-bit words.
class Assembler {
public:
    // Assemble an entire file.  Returns true if no errors.
    bool assemble_file(const std::string& filename,
                       std::vector<uint16_t>& program);

    // Assemble a vector of source lines (useful for tests).
    bool assemble_lines(const std::vector<std::string>& lines,
                        std::vector<uint16_t>& program);

    // Encode a *valid* Instruction struct into a 16-bit machine word.
    static uint16_t encode(const Instruction& inst);

    // Access accumulated errors.
    const std::vector<AsmError>& errors() const { return errors_; }

private:
    std::vector<AsmError> errors_;

    // Parse one source line into an encoded word.
    // Returns true on success; on failure, pushes to errors_.
    bool parse_line(const std::string& raw, int line_num, uint16_t& out);

    // ── Tiny string helpers (defined in assembler.cpp) ──────────────────────
    static std::string             trim(const std::string& s);
    static std::string             to_upper(const std::string& s);
    static std::vector<std::string> tokenize(const std::string& line);
    static bool                     parse_register(const std::string& tok,
                                                   uint8_t& reg);
    static bool                     parse_immediate(const std::string& tok,
                                                    int& value);
};

} // namespace mini_risc
