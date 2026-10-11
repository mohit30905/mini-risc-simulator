// ============================================================================
// main.cpp — Mini RISC Simulator Command-Line Interface
// ============================================================================
// Usage:
//   mini_risc assemble <file.asm>     Assemble and print encoded instructions
//   mini_risc run      <file.asm>     Assemble, run, show final registers
//   mini_risc trace    <file.asm>     Assemble, run with step-by-step trace
//   mini_risc test                    Run the full verification test suite
//   mini_risc help                    Show usage information
// ============================================================================
#include "isa.hpp"
#include "assembler.hpp"
#include "decoder.hpp"
#include "cpu.hpp"
#include "verifier.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace mini_risc;

// ── Print usage ─────────────────────────────────────────────────────────────
static void print_usage(const char* argv0) {
    std::cout << "\nMini RISC ISA Simulator & Verification Framework\n"
              << "================================================\n\n"
              << "Usage:\n"
              << "  " << argv0 << " assemble <file.asm>   "
                 "Assemble and print machine code\n"
              << "  " << argv0 << " run      <file.asm>   "
                 "Assemble and execute, show registers\n"
              << "  " << argv0 << " trace    <file.asm>   "
                 "Assemble and execute with trace\n"
              << "  " << argv0 << " test                  "
                 "Run all verification tests\n"
              << "  " << argv0 << " help                  "
                 "Show this message\n\n";
}

// ── Assemble a file and return the program ──────────────────────────────────
static bool do_assemble(const std::string& filename,
                        std::vector<uint16_t>& prog,
                        bool print_output) {
    Assembler as;
    bool ok = as.assemble_file(filename, prog);

    if (!ok) {
        std::cerr << "\nAssembly errors in " << filename << ":\n";
        for (const auto& e : as.errors()) {
            std::cerr << "  Line " << e.line << ": " << e.message << "\n";
        }
        return false;
    }

    if (print_output) {
        std::cout << "\nAssembled " << prog.size()
                  << " instruction(s) from " << filename << ":\n\n";
        std::cout << "  Addr  |  Hex   |  Binary           |  Disassembly\n";
        std::cout << "  ------+--------+-------------------+-----------------\n";
        for (size_t i = 0; i < prog.size(); ++i) {
            uint16_t w = prog[i];
            // Binary string
            std::string bin;
            for (int b = 15; b >= 0; --b) {
                bin += ((w >> b) & 1) ? '1' : '0';
                if (b == 12 || b == 9 || b == 6 || b == 3) bin += ' ';
            }
            std::cout << "  " << std::setw(4) << i << "  |  0x"
                      << std::hex << std::setw(4) << std::setfill('0') << w
                      << std::dec << std::setfill(' ')
                      << "  |  " << bin
                      << "  |  " << Decoder::disassemble(w) << "\n";
        }
        std::cout << "\n";
    }
    return true;
}

// ── Main ────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    // ── help ────────────────────────────────────────────────────────────────
    if (cmd == "help" || cmd == "--help" || cmd == "-h") {
        print_usage(argv[0]);
        return 0;
    }

    // ── test ────────────────────────────────────────────────────────────────
    if (cmd == "test") {
        Verifier v;
        bool all_pass = v.run_all();
        v.print_report();
        return all_pass ? 0 : 1;
    }

    // Remaining commands need a filename
    if (argc < 3) {
        std::cerr << "Error: '" << cmd << "' requires a filename.\n";
        print_usage(argv[0]);
        return 1;
    }
    std::string filename = argv[2];

    // ── assemble ────────────────────────────────────────────────────────────
    if (cmd == "assemble") {
        std::vector<uint16_t> prog;
        return do_assemble(filename, prog, true) ? 0 : 1;
    }

    // ── run ─────────────────────────────────────────────────────────────────
    if (cmd == "run") {
        std::vector<uint16_t> prog;
        if (!do_assemble(filename, prog, true)) return 1;

        CPU cpu;
        cpu.load(prog);
        bool halted = cpu.run();
        cpu.print_registers();

        if (!halted) {
            std::cerr << "\nWarning: program did not HALT normally "
                      << "(stopped after " << cpu.state().steps << " steps).\n";
        }
        return 0;
    }

    // ── trace ───────────────────────────────────────────────────────────────
    if (cmd == "trace") {
        std::vector<uint16_t> prog;
        if (!do_assemble(filename, prog, true)) return 1;

        CPU cpu;
        cpu.load(prog);
        bool halted = cpu.run(/*trace=*/true);
        cpu.print_trace();
        cpu.print_registers();

        if (!halted) {
            std::cerr << "\nWarning: program did not HALT normally.\n";
        }
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    print_usage(argv[0]);
    return 1;
}
