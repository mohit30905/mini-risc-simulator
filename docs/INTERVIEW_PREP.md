# Interview Preparation: Mini RISC ISA Simulator & Verification

This guide prepares you to discuss this project confidently in technical interviews, specifically targeting roles in **Processor Simulation, Architecture Modeling, and Compiler Verification** (such as NVIDIA's Compiler Verification team).

---

## 1. The 60-Second Elevator Pitch

> *"To bridge my digital logic experience with processor software tools, I designed and built a 16-bit Mini RISC instruction set architecture, CPU simulator, assembler, and automated verification framework in modern C++17. The architecture defines 8 general-purpose registers, 4-bit opcodes, and three distinct instruction formats covering arithmetic, bitwise, shift, and control operations. 
> 
> I implemented a two-pass capable assembler with input validation and diagnostics, a cycle-accurate fetch-decode-execute simulator with step tracing, and a comprehensive verification suite featuring 110 automated tests. Crucially for compiler verification, the test framework tests against an independently calculated reference model to avoid shared-implementation bias, verifying boundary wrap-around, immediate sign-extension, and decode/encode round trips. This project gave me hands-on mastery over binary instruction encodings, architectural state mutations, and differential testing methodologies."*

---

## 2. 20 Likely Technical Interview Questions & Answers

### Architecture & ISA
**Q1: Why did you choose a fixed 16-bit instruction width instead of variable-length instructions like x86?**  
*Answer:* Fixed-width encoding is a hallmark of RISC architectures. It simplifies instruction fetch (each instruction is exactly one 16-bit word), eliminates complex byte-stream pre-decoders, guarantees predictable instruction alignment, and allows bit fields like opcode and register indices to remain at fixed positions.

**Q2: How do your three instruction formats partition the 16 bits without ambiguity?**  
*Answer:* Bits [15:12] are reserved strictly for the 4-bit opcode across all formats. Once the opcode is decoded, the format is uniquely identified: R-Type allocates 3 bits each to Rd, Rs1, and Rs2; I-Type allocates 3 bits to Rd and 9 bits to the immediate; S-Type allocates 3 bits to Rd, 3 bits to Rs, and 4 bits to shift amount. Unused bits are zeroed and validated.

**Q3: Why is R0 writable in your architecture when MIPS and RISC-V hardwire R0 to zero?**  
*Answer:* In architectures with only 8 registers, sacrificing one register to a hardwired zero reduces general-purpose storage by 12.5%. Making R0 writable maximizes register availability for small programs while remaining internally consistent across the assembler, decoder, CPU state, and reference model.

**Q4: How does your immediate encoding handle negative numbers?**  
*Answer:* I-Type allocates 9 bits for the immediate in two's complement format ($[-256, +255]$). Bit 8 is the sign bit. During decoding, if bit 8 is 1, the decoder sign-extends by bitwise-ORing `0xFE00` to fill bits [15:9], yielding a negative 16-bit signed integer (e.g., `-1` becomes `0xFFFF`).

**Q5: What happens during arithmetic overflow or underflow?**  
*Answer:* All arithmetic behaves as modulo $2^{16}$ wrap-around without hardware traps. For example, `0xFFFF + 1` wraps to `0x0000`, and `0x0000 - 1` wraps to `0xFFFF`. This mirrors typical unsigned/signed integer wrapping in hardware ALUs.

---

### Simulator & Execution Loop
**Q6: Walk me through the Fetch-Decode-Execute cycle in your CPU.**  
*Answer:* In `CPU::step()`:
1. **Fetch:** Reads `memory_[state_.pc]`, verifying that PC is within memory bounds.
2. **Decode:** Calls `Decoder::decode(word)` to extract opcode and operands into an `Instruction` struct.
3. **Execute:** Calls `execute(inst)` which switches on opcode, performs the ALU/register mutation, and advances `state_.pc++` (unless HALT was encountered).

**Q7: How does your simulator prevent infinite execution loops?**  
*Answer:* The `CPU::run()` loop enforces a step limit `MAX_EXECUTION_STEPS = 10000`. If a program lacks a HALT instruction or gets caught in a cycle, the simulator halts cleanly and issues a warning.

**Q8: Why does the HALT instruction not increment the Program Counter?**  
*Answer:* When a CPU halts, the PC should point to the instruction that halted the machine. Incrementing the PC would make it point to uninitialized memory or out-of-bounds addresses, corrupting post-execution state inspection.

**Q9: How is execution tracing implemented?**  
*Answer:* Before executing an instruction, the CPU captures old register state. After execution, it records the step number, PC, instruction word hex, disassembly string, which register changed, and the transition ($old \rightarrow new$) into a `TraceEntry` struct.

---

### Assembler & Encoding
**Q10: How does your assembler handle comments and whitespace?**  
*Answer:* In `Assembler::tokenize()`, lines are trimmed and comments starting with `;` or `#` are stripped before tokenization. Commas are replaced with spaces, allowing flexible syntax like `ADD R1, R2, R3` or `ADD R1 R2 R3`.

**Q11: How do you validate register numbers and immediate bounds?**  
*Answer:* Register tokens must strictly match `R[0-7]` (case-insensitive). Immediates are parsed using `std::stol` or `std::stoul` (supporting decimal and `0x` hex) and checked against range `[-256, 255]`. Any out-of-bound input generates a localized `AsmError` with the line number.

**Q12: How do you pack negative numbers into a bitfield in C++?**  
*Answer:* In `Assembler::encode()`, the `int16_t imm` is cast to `uint16_t` and masked with `IMM_MASK` (`0x01FF`). This truncates any higher sign bits from C++'s 16/32-bit integer representation, isolating only the 9 immediate bits for the bitwise OR.

---

### Verification & Testing
**Q13: What is a Reference Model and why is it essential in your project?**  
*Answer:* A reference model is an independent, high-level behavioral specification of the expected hardware/instruction semantics. If you test your simulator by checking `cpu.reg(3) == Assembler::encode(...)` or using the same ALU functions, a bug in the shared implementation goes undetected. My `ref::` namespace uses standalone arithmetic and bit-packing code.

**Q14: What is an encode-decode round trip test?**  
*Answer:* It tests that for any valid instruction: $\text{Decode}(\text{Encode}(inst)) \equiv inst$. It validates that no bits are lost, shifted improperly, or misinterpreted across format boundaries.

**Q15: How did you test edge cases and boundaries?**  
*Answer:* I explicitly wrote tests for:
- Maximum/minimum immediates (`+255`, `-256`, `0`, `-1`).
- Arithmetic wrap (`0xFFFF + 1 = 0x0000`, `0x0000 - 1 = 0xFFFF`).
- Maximum shifts (shift by 0 and shift by 15).
- Unused bit isolation and invalid opcodes (`0xF000`).

**Q16: How do you verify negative immediate sign-extension?**  
*Answer:* A test executes `MOVI R1, -1` and verifies that `cpu.reg(1) == 0xFFFF`. If the sign bit (bit 8) was not extended into bits [15:9], `R1` would incorrectly be `0x01FF` ($511$).

---

### Compiler Verification & Role Relevance
**Q17: Why is this an ISA simulator rather than a compiler or full assembler?**  
*Answer:* An ISA simulator models processor execution state. A compiler translates high-level code (like C or CUDA) into an intermediate representation (IR) and then into assembly through optimization passes and register allocation. This simulator serves as the **execution target and reference validation engine** that a compiler verification pipeline would test against.

**Q18: How does this project relate to NVIDIA Compiler Verification?**  
*Answer:* NVIDIA Compiler Verification engineers verify that compilers like NVCC produce correct PTX (virtual assembly) and SASS (machine assembly). They use architectural simulators, instruction decoders, and reference models to verify that optimized machine instructions produce bit-accurate results compared to golden models.

**Q19: What are the primary limitations of this project compared to NVIDIA hardware?**  
*Answer:* Mini RISC is a scalar CPU without SIMT (warp execution, thread divergence), memory hierarchies (shared memory, caches), branch instructions, or floating-point/tensor operations. It demonstrates core principles of ISA verification without GPU-specific complexity.

**Q20: What would you add next to make this closer to a real verification testbench?**  
*Answer:* 
1. Branch instructions (`JMP`, `BEQ`, `BNE`) to test control flow.
2. Memory load/store (`LD`, `ST`) instructions.
3. A randomized instruction sequence generator (fuzzer) comparing the simulator against a hardware model in Verilog via Verilator.

---

## 3. Five Instruction-Tracing Exercises

### Exercise 1: Basic Register Dependency
```text
MOVI R1, 5
MOVI R2, 3
ADD  R1, R1, R2
SUB  R2, R1, R2
HALT
```
*Trace:*
- Step 0: `R1 = 5`
- Step 1: `R2 = 3`
- Step 2: `R1 = R1 + R2 = 5 + 3 = 8`
- Step 3: `R2 = R1 - R2 = 8 - 3 = 5`
- Final: `R1 = 8`, `R2 = 5`.

### Exercise 2: Bitwise Masking
```text
MOVI R1, 0xFF
MOVI R2, 0x0F
XOR  R3, R1, R2
AND  R4, R3, R1
HALT
```
*Trace:*
- `R1 = 0x00FF` (255)
- `R2 = 0x000F` (15)
- `R3 = 0x00FF ^ 0x000F = 0x00F0` (240)
- `R4 = 0x00F0 & 0x00FF = 0x00F0` (240)
- Final: `R3 = 240`, `R4 = 240`.

### Exercise 3: Sign-Extension Addition
```text
MOVI R1, -10
MOVI R2, 10
ADD  R3, R1, R2
HALT
```
*Trace:*
- `R1 = -10` $\rightarrow$ `0xFFF6` (65526)
- `R2 = 10`  $\rightarrow$ `0x000A` (10)
- `R3 = 0xFFF6 + 0x000A = 0x10000 & 0xFFFF = 0x0000` (0)
- Final: `R3 = 0`.

### Exercise 4: Left Shift Overflow Wrap
```text
MOVI R1, 1
SHL  R2, R1, 15
SHL  R3, R2, 1
HALT
```
*Trace:*
- `R1 = 0x0001`
- `R2 = 1 << 15 = 0x8000` (32768)
- `R3 = 0x8000 << 1 = 0x10000 & 0xFFFF = 0x0000` (0)
- Final: `R2 = 32768`, `R3 = 0`.

### Exercise 5: Reusing R0
```text
MOVI R0, 100
ADD  R0, R0, R0
HALT
```
*Trace:*
- `R0 = 100`
- `R0 = 100 + 100 = 200`
- Final: `R0 = 200` (confirms R0 is fully writable).

---

## 4. Five Realistic Debugging Scenarios

### Scenario 1: Immediate Sign Bug
- **Symptom:** `MOVI R1, -1` causes `ADD R2, R1, R1` to produce `1022` instead of `-2` (`0xFFFE`).
- **Cause:** The decoder extracted `word & 0x01FF` (`511`) without sign extension. In 16-bit math, $511 + 511 = 1022$.
- **Fix:** Check bit 8; if set, OR with `0xFE00` so `-1` becomes `0xFFFF`.

### Scenario 2: C++ Arithmetic Shift Right Bug
- **Symptom:** `SHR` on negative values shifts in `1`s instead of `0`s.
- **Cause:** In C++, right-shifting a signed integer (`int16_t`) performs an **arithmetic** shift (preserving sign bit).
- **Fix:** Explicitly cast operands to `uint16_t` before shifting: `static_cast<uint16_t>(regs[rs] >> shamt)`.

### Scenario 3: Missing HALT Runaway
- **Symptom:** Assembly without `HALT` accesses out-of-bounds memory.
- **Cause:** PC increments indefinitely past the end of the loaded program vector.
- **Fix:** Add bounds checking in `step()`: `if (state_.pc >= memory_.size()) { halted = true; return false; }`.

### Scenario 4: Overlapping Bitfield Bug
- **Symptom:** In S-Type instructions, shift amounts greater than 7 corrupt the source register `Rs`.
- **Cause:** Incorrect bit mask or shift offset in the encoder (e.g., shifting by 3 instead of 2, colliding with Rs bits [8:6]).
- **Fix:** Verified bit alignment: `Rs` is in [8:6], `shamt` is in [5:2] (4 bits), bits [1:0] are unused.

### Scenario 5: Case-Sensitive Register Parsing
- **Symptom:** `movi r1, 10` fails with "Invalid register: r1".
- **Cause:** Tokenizer did not convert register strings to uppercase.
- **Fix:** Apply `std::transform(..., ::toupper)` before checking `r[0] == 'R'`.

---

## 5. Resume Entry (Accurate & Truthful)

```text
Mini RISC ISA Simulator & Verification Framework (C++17)
• Designed and implemented a 16-bit RISC ISA specification featuring 8 registers, 
  3 instruction formats, and an opcode-based fetch-decode-execute simulator.
• Developed a two-pass assembler with syntax tokenization, immediate boundary 
  validation, and binary machine code generation.
• Built an automated verification framework featuring an independent software reference 
  model and 110 unit/integration tests verifying arithmetic boundaries, sign extension, 
  and encode/decode round trips.
```
