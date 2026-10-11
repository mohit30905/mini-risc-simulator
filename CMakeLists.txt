cmake_minimum_required(VERSION 3.10)
project(mini_risc VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Main executable ─────────────────────────────────────────────────────────
add_executable(mini_risc
    src/main.cpp
    src/assembler.cpp
    src/decoder.cpp
    src/cpu.cpp
    src/verifier.cpp
)
target_include_directories(mini_risc PRIVATE src)

# ── Standalone test executable ──────────────────────────────────────────────
add_executable(run_tests
    tests/test_main.cpp
    src/assembler.cpp
    src/decoder.cpp
    src/cpu.cpp
    src/verifier.cpp
)
target_include_directories(run_tests PRIVATE src)
