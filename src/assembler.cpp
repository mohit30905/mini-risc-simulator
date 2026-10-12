// ============================================================================
// assembler.cpp — Mini RISC Assembler Implementation
// ============================================================================
// Workflow:  source text  →  tokenize  →  validate  →  encode  →  uint16_t
// ============================================================================
#include "assembler.hpp"

#include <fstream>
#include <sstream>
#include <cctype>

namespace mini_risc {

// ════════════════════════════════════════════════════════════════════════════
// String helpers
// ════════════════════════════════════════════════════════════════════════════

std::string Assembler::trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Assembler::to_upper(const std::string& s) {
    std::string u = s;
    std::transform(u.begin(), u.end(), u.begin(), ::toupper);
    return u;
}

// Tokenize a line: strip comment (everything after ';' or '#'),
// then split on whitespace and commas.
std::vector<std::string> Assembler::tokenize(const std::string& line) {
    // Strip comment
    std::string clean = line;
    auto comment_pos = clean.find(';');
    if (comment_pos != std::string::npos) clean = clean.substr(0, comment_pos);
    comment_pos = clean.find('#');
    if (comment_pos != std::string::npos) clean = clean.substr(0, comment_pos);

    // Replace commas with spaces for uniform splitting
    for (char& c : clean) {
        if (c == ',') c = ' ';
    }

    std::vector<std::string> tokens;
    std::istringstream iss(clean);
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(tok);
    }
    return tokens;
}

// Parse a register token like "R0" … "R7".  Case-insensitive.
bool Assembler::parse_register(const std::string& tok, uint8_t& reg) {
    std::string u = to_upper(trim(tok));
    if (u.size() != 2 || u[0] != 'R') return false;
    if (u[1] < '0' || u[1] > '7') return false;
    reg = static_cast<uint8_t>(u[1] - '0');
    return true;
}

// Parse an immediate value: decimal (possibly negative) or hex (0x…).
bool Assembler::parse_immediate(const std::string& tok, int& value) {
    std::string t = trim(tok);
    if (t.empty()) return false;
    try {
        size_t pos = 0;
        // Detect hex prefix
        if (t.size() > 2 && t[0] == '0' && (t[1] == 'x' || t[1] == 'X')) {
            unsigned long v = std::stoul(t, &pos, 16);
            if (pos != t.size()) return false;
            value = static_cast<int>(v);
        } else {
            long v = std::stol(t, &pos, 10);
            if (pos != t.size()) return false;
            value = static_cast<int>(v);
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ════════════════════════════════════════════════════════════════════════════
// Encode: Instruction struct → 16-bit machine word
// ════════════════════════════════════════════════════════════════════════════
uint16_t Assembler::encode(const Instruction& inst) {
    uint16_t word = 0;
    uint8_t op = static_cast<uint8_t>(inst.opcode);
    word |= static_cast<uint16_t>(op & 0x0F) << 12;   // bits [15:12]

    Format fmt = get_format(inst.opcode);
    switch (fmt) {
        case Format::R_TYPE:
            word |= static_cast<uint16_t>(inst.rd  & 0x07) << 9;   // [11:9]
            word |= static_cast<uint16_t>(inst.rs1 & 0x07) << 6;   // [8:6]
            word |= static_cast<uint16_t>(inst.rs2 & 0x07) << 3;   // [5:3]
            break;
        case Format::I_TYPE:
            word |= static_cast<uint16_t>(inst.rd & 0x07) << 9;    // [11:9]
            word |= static_cast<uint16_t>(inst.imm) & IMM_MASK;    // [8:0]
            break;
        case Format::S_TYPE:
            word |= static_cast<uint16_t>(inst.rd & 0x07) << 9;    // [11:9]
            word |= static_cast<uint16_t>(inst.rs1 & 0x07) << 6;   // [8:6]
            word |= static_cast<uint16_t>(inst.shamt & 0x0F) << 2; // [5:2]
            break;
        case Format::H_TYPE:
            // All remaining bits stay 0
            break;
        default:
            break;
    }
    return word;
}

// ════════════════════════════════════════════════════════════════════════════
// parse_line: validate syntax + semantics of one source line
// ════════════════════════════════════════════════════════════════════════════
bool Assembler::parse_line(const std::string& raw, int line_num, uint16_t& out) {
    auto tokens = tokenize(raw);
    if (tokens.empty()) {
        // Blank / comment-only line — skip silently
        out = 0;
        return false;   // not an error, but nothing to emit
    }

    std::string mnemonic = to_upper(tokens[0]);
    Opcode op = string_to_opcode(mnemonic);
    if (op == Opcode::INVALID) {
        errors_.push_back({line_num, "Unknown instruction: " + tokens[0]});
        return false;
    }

    Instruction inst;
    inst.opcode = op;
    Format fmt  = get_format(op);

    // ── HALT ────────────────────────────────────────────────────────────────
    if (fmt == Format::H_TYPE) {
        if (tokens.size() != 1) {
            errors_.push_back({line_num, "HALT takes no operands"});
            return false;
        }
        inst.valid = true;
        out = encode(inst);
        return true;
    }

    // ── MOVI Rd, imm ───────────────────────────────────────────────────────
    if (fmt == Format::I_TYPE) {
        if (tokens.size() != 3) {
            errors_.push_back({line_num,
                "MOVI requires 2 operands: MOVI Rd, immediate"});
            return false;
        }
        if (!parse_register(tokens[1], inst.rd)) {
            errors_.push_back({line_num,
                "Invalid register: " + tokens[1] + " (expected R0–R7)"});
            return false;
        }
        int imm_val = 0;
        if (!parse_immediate(tokens[2], imm_val)) {
            errors_.push_back({line_num,
                "Invalid immediate value: " + tokens[2]});
            return false;
        }
        if (imm_val < IMM_MIN || imm_val > IMM_MAX) {
            errors_.push_back({line_num,
                "Immediate " + std::to_string(imm_val) +
                " out of range [" + std::to_string(IMM_MIN) +
                " … " + std::to_string(IMM_MAX) + "]"});
            return false;
        }
        inst.imm   = static_cast<int16_t>(imm_val);
        inst.valid  = true;
        out = encode(inst);
        return true;
    }

    // ── R-type:  OP Rd, Rs1, Rs2 ───────────────────────────────────────────
    if (fmt == Format::R_TYPE) {
        if (tokens.size() != 4) {
            errors_.push_back({line_num,
                mnemonic + " requires 3 operands: " +
                mnemonic + " Rd, Rs1, Rs2"});
            return false;
        }
        if (!parse_register(tokens[1], inst.rd)) {
            errors_.push_back({line_num,
                "Invalid destination register: " + tokens[1]});
            return false;
        }
        if (!parse_register(tokens[2], inst.rs1)) {
            errors_.push_back({line_num,
                "Invalid source register 1: " + tokens[2]});
            return false;
        }
        if (!parse_register(tokens[3], inst.rs2)) {
            errors_.push_back({line_num,
                "Invalid source register 2: " + tokens[3]});
            return false;
        }
        inst.valid = true;
        out = encode(inst);
        return true;
    }

    // ── S-type:  SHL/SHR Rd, Rs, amount ────────────────────────────────────
    if (fmt == Format::S_TYPE) {
        if (tokens.size() != 4) {
            errors_.push_back({line_num,
                mnemonic + " requires 3 operands: " +
                mnemonic + " Rd, Rs, amount"});
            return false;
        }
        if (!parse_register(tokens[1], inst.rd)) {
            errors_.push_back({line_num,
                "Invalid destination register: " + tokens[1]});
            return false;
        }
        if (!parse_register(tokens[2], inst.rs1)) {  // rs1 holds the source
            errors_.push_back({line_num,
                "Invalid source register: " + tokens[2]});
            return false;
        }
        int shamt_val = 0;
        if (!parse_immediate(tokens[3], shamt_val)) {
            errors_.push_back({line_num,
                "Invalid shift amount: " + tokens[3]});
            return false;
        }
        if (shamt_val < 0 || shamt_val > SHAMT_MAX) {
            errors_.push_back({line_num,
                "Shift amount " + std::to_string(shamt_val) +
                " out of range [0 … " + std::to_string(SHAMT_MAX) + "]"});
            return false;
        }
        inst.shamt = static_cast<uint8_t>(shamt_val);
        inst.valid = true;
        out = encode(inst);
        return true;
    }

    errors_.push_back({line_num, "Internal error: unhandled format"});
    return false;
}

// ════════════════════════════════════════════════════════════════════════════
// Public: assemble from file
// ════════════════════════════════════════════════════════════════════════════
bool Assembler::assemble_file(const std::string& filename,
                              std::vector<uint16_t>& program) {
    errors_.clear();
    program.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        errors_.push_back({0, "Cannot open file: " + filename});
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return assemble_lines(lines, program);
}

// ════════════════════════════════════════════════════════════════════════════
// Public: assemble from vector of lines
// ════════════════════════════════════════════════════════════════════════════
bool Assembler::assemble_lines(const std::vector<std::string>& lines,
                               std::vector<uint16_t>& program) {
    errors_.clear();
    program.clear();

    for (size_t i = 0; i < lines.size(); ++i) {
        int line_num = static_cast<int>(i + 1);
        auto tokens = tokenize(lines[i]);
        if (tokens.empty()) continue;   // blank / comment

        uint16_t word = 0;
        if (parse_line(lines[i], line_num, word)) {
            program.push_back(word);
        }
    }
    return errors_.empty();
}

} // namespace mini_risc
