# Mini RISC ISA Simulator & Automated Verification Framework

A lightweight, robust, educational 16-bit RISC processor simulator, assembler, disassembler, and automated verification framework implemented in **modern C++17**.

---

## Features

- **Custom 16-bit RISC ISA**: 8 general-purpose registers (R0–R7), 4-bit opcodes, 3 instruction formats (R-Type, I-Type, S-Type) + HALT.
- **Two-Pass-Capable Assembler**: Reads `.asm` files, tokenizes syntax, handles decimal/hex immediates, validates register indices and immediate boundaries, and emits binary machine words with descriptive diagnostics.
- **Instruction Decoder**: Decodes 16-bit words into structured opcode and operand fields with sign extension and automatic disassembly.
- **CPU Simulator**: Implements a cycle-accurate fetch-decode-execute loop with register file inspection, execution step tracking, safety limits, and execution tracing.
- **Verification Framework**: Complete automated test suite using an **independent reference model** (no shared helper bugs) with 110 automated unit and system tests covering encode/decode round trips, arithmetic boundaries, bitwise operations, shifts, error handling, and program execution.

---

## Directory Structure

```text
mini-risc-simulator/
├── CMakeLists.txt          # Standard CMake build definition
├── README.md               # Project overview and quickstart guide
├── isa_spec.md             # Formal ISA bitfield and semantic specification
├── src/
│   ├── isa.hpp             # Opcodes, formats, constants, Instruction struct
│   ├── assembler.hpp/cpp   # Text parsing, syntax validation, binary encoding
│   ├── decoder.hpp/cpp     # Bit unpacking, sign extension, disassembly
│   ├── cpu.hpp/cpp         # Fetch-decode-execute loop, register file, tracing
│   ├── verifier.hpp/cpp    # Independent reference model & 110 automated tests
│   └── main.cpp            # Command-line interface driver
├── examples/
│   ├── basic.asm           # Basic arithmetic (ADD, SUB, XOR)
│   ├── bitwise.asm         # Logical operations (AND, OR, XOR with hex)
│   ├── shifts.asm          # Logical left/right shifts (SHL, SHR)
│   ├── boundary.asm        # Overflow, underflow, sign-extension boundaries
│   └── invalid.asm         # Malformed assembly tests for diagnostic checks
├── tests/
│   └── test_main.cpp       # Standalone test runner
└── docs/
    ├── LEARN_THE_PROJECT.md # Deep-dive beginner-friendly tutorial
    └── INTERVIEW_PREP.md    # 60s pitch, 20 interview Q&As, trace exercises
```

---

## Quickstart & Build Instructions

### Prerequisites
- C++17 compliant compiler (`g++` via MinGW on Windows, `clang++`, or MSVC)
- CMake 3.10+ (optional, or use direct `g++` commands)

### Compilation via g++ (MinGW / Windows / Linux)

```powershell
# Build main CLI tool
g++ -std=c++17 -Wall -Wextra -O2 -o mini_risc.exe src/main.cpp src/assembler.cpp src/decoder.cpp src/cpu.cpp src/verifier.cpp -I src

# Build standalone test executable
g++ -std=c++17 -Wall -Wextra -O2 -o run_tests.exe tests/test_main.cpp src/assembler.cpp src/decoder.cpp src/cpu.cpp src/verifier.cpp -I src
```

### Compilation via CMake

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## CLI Usage

### 1. Assemble an Assembly File
Encodes the source file into binary and hexadecimal machine instructions:
```powershell
.\mini_risc.exe assemble examples\basic.asm
```

### 2. Execute a Program
Assembles and runs the program, printing final CPU state and register contents:
```powershell
.\mini_risc.exe run examples\basic.asm
```

### 3. Step-by-Step Execution Trace
Traces every instruction step, PC address, instruction word, disassembly, and register mutations:
```powershell
.\mini_risc.exe trace examples\basic.asm
```

### 4. Run Automated Verification Tests
Executes all 110 automated verification checks and reports pass/fail:
```powershell
.\mini_risc.exe test
# Or run the standalone test binary:
.\run_tests.exe
```

---

## Test Verification Summary
All 110 test assertions pass:
```text
╔══════════════════════════════════════════════════╗
║         MINI RISC VERIFICATION REPORT           ║
╚══════════════════════════════════════════════════╝

  Total: 110  |  Passed: 110  |  Failed: 0
  ✓ ALL TESTS PASSED
```
