; ============================================================================
; invalid.asm — Intentionally broken program (tests error reporting)
; ============================================================================
; Every line below contains an error.  The assembler should reject each one
; with a clear diagnostic message.
; ============================================================================

FOO  R1, R2       ; ERROR: unknown instruction "FOO"
ADD  R1, R2       ; ERROR: ADD needs 3 operands, only 2 given
MOVI R9, 10       ; ERROR: R9 is not a valid register (only R0–R7)
MOVI R1, 300      ; ERROR: immediate 300 exceeds range [-256 … 255]
SHL  R1, R2, 16   ; ERROR: shift amount 16 exceeds range [0 … 15]
HALT R1           ; ERROR: HALT takes no operands
