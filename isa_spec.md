# Mini RISC Instruction Set Architecture (ISA) Specification

**Version:** 1.0  
**Word Size:** 16-bit  
**Register Count:** 8 General-Purpose Registers (R0–R7)  
**Byte / Word Order:** Word-addressed instruction memory (16-bit words)

---

## 1. Architectural State

### 1.1 Registers
The processor contains eight general-purpose registers: `R0` through `R7`.
- **Width:** 16 bits each (values `0x0000` to `0xFFFF`, representing unsigned 0–65535 or signed two's complement -32768 to 32767).
- **R0 Semantics:** Unlike architectures where R0 is hardwired to zero (e.g., MIPS), in Mini RISC **R0 is a standard, fully writable general-purpose register**. All registers initialize to `0x0000` at CPU reset.
- **Program Counter (PC):** 16-bit register pointing to the index of the next instruction word in instruction memory. Starts at `PC = 0`.
- **Instruction Memory:** A linear array of 16-bit instruction words indexed directly by PC.

---

## 2. Instruction Formats

Every instruction is strictly **16 bits** wide. The most significant 4 bits (`[15:12]`) always identify the **Opcode**.

### Format Summary Table

| Format | Bits [15:12] | Bits [11:9] | Bits [8:6] | Bits [5:3] | Bits [2:0] | Instructions |
|---|---|---|---|---|---|---|
| **R-Type** | Opcode (4b) | Rd (3b) | Rs1 (3b) | Rs2 (3b) | Unused `000` (3b) | `ADD`, `SUB`, `AND`, `OR`, `XOR` |
| **I-Type** | Opcode (4b) | Rd (3b) | Imm[8:6] | Imm[5:3] | Imm[2:0] | `MOVI` (Imm9 in bits `[8:0]`) |
| **S-Type** | Opcode (4b) | Rd (3b) | Rs (3b) | Shamt (4b) in `[5:2]` | Unused `00` (2b) | `SHL`, `SHR` |
| **H-Type** | Opcode (4b) | Unused `000 000 000 000` (12b) | — | — | — | `HALT` |

---

## 3. Instruction Set Details

### 3.1 Opcode Encoding Table

| Mnemonic | Opcode (Hex) | Opcode (Binary) | Format | Assembly Syntax | Operation Semantics |
|---|---|---|---|---|---|
| **HALT** | `0x0` | `0000` | H-Type | `HALT` | Stop CPU execution. PC does not advance. |
| **MOVI** | `0x1` | `0001` | I-Type | `MOVI Rd, imm` | `Rd = SignExtend16(imm9)` |
| **ADD**  | `0x2` | `0010` | R-Type | `ADD Rd, Rs1, Rs2` | `Rd = (Rs1 + Rs2) & 0xFFFF` |
| **SUB**  | `0x3` | `0011` | R-Type | `SUB Rd, Rs1, Rs2` | `Rd = (Rs1 - Rs2) & 0xFFFF` |
| **AND**  | `0x4` | `0100` | R-Type | `AND Rd, Rs1, Rs2` | `Rd = Rs1 & Rs2` |
| **OR**   | `0x5` | `0101` | R-Type | `OR Rd, Rs1, Rs2`  | `Rd = Rs1 \| Rs2` |
| **XOR**  | `0x6` | `0110` | R-Type | `XOR Rd, Rs1, Rs2` | `Rd = Rs1 ^ Rs2` |
| **SHL**  | `0x7` | `0111` | S-Type | `SHL Rd, Rs, amt`  | `Rd = (Rs << amt) & 0xFFFF` |
| **SHR**  | `0x8` | `1000` | S-Type | `SHR Rd, Rs, amt`  | `Rd = Rs >> amt` (Logical shift right) |

---

## 4. Arithmetic, Bitwise, and Shift Semantics

### 4.1 Word Wrap and Overflow
- All 16-bit arithmetic operations wrap modulo $2^{16} = 65536$.
- Overflow: `0xFFFF + 1` wraps to `0x0000`.
- Underflow: `0x0000 - 1` wraps to `0xFFFF` ($65535$).
- No hardware trap is triggered on overflow or underflow.

### 4.2 Immediate Field (I-Type)
- Immediate values occupy 9 bits (`[8:0]`).
- Encoded as **two's-complement signed integers** spanning `[-256, +255]` (`0x100` to `0x0FF`).
- Bit 8 is the sign bit (`IMM_SIGN = 0x0100`).
- Sign extension rule:
  $$\text{SignExtend}(imm9) = \begin{cases} imm9 \mid 0\text{xFE00} & \text{if bit 8 is 1} \\ imm9 & \text{if bit 8 is 0} \end{cases}$$
- Example: `-1` (`0x1FF`) sign-extends to `0xFFFF`. `-256` (`0x100`) sign-extends to `0xFF00`. `10` (`0x00A`) sign-extends to `0x000A`.

### 4.3 Shift Semantics (S-Type)
- The shift amount `shamt` occupies 4 bits (`[5:2]`), allowing amounts from `0` to `15`.
- `SHL` performs a **logical left shift**, shifting in zeros on the right. Bits shifted past bit 15 are discarded.
- `SHR` performs a **logical right shift**, shifting in zeros on the left (zero-fill).
- A shift by 0 acts as a register copy (`Rd = Rs`).

### 4.4 HALT and Program Termination
- `HALT` sets the CPU `halted` flag to `true`.
- The PC remains on the `HALT` instruction.
- The simulator also enforces a safety threshold of `MAX_EXECUTION_STEPS = 10000` steps to avoid runaway loops in malformed programs.
