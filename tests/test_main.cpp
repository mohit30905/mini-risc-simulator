// ============================================================================
// tests/test_main.cpp — Standalone test runner
// ============================================================================
// This is an independent entry point that runs the full verification suite.
// Build:  g++ -std=c++17 -I../src ../src/assembler.cpp ../src/decoder.cpp
//         ../src/cpu.cpp ../src/verifier.cpp test_main.cpp -o run_tests
// Or use: mini_risc test   (which calls the same Verifier internally)
// ============================================================================
#include "../src/verifier.hpp"
#include <iostream>

int main() {
    std::cout << "Mini RISC — Standalone Test Runner\n"
              << "===================================\n";

    mini_risc::Verifier v;
    bool all_pass = v.run_all();
    v.print_report();

    return all_pass ? 0 : 1;
}
