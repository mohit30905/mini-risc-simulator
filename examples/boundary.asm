; ============================================================================
; boundary.asm — Boundary-value / edge-case demo
; ============================================================================
; Tests:
;   Overflow:   0xFFFF + 1 wraps to 0          → R3 = 0
;   Underflow:  0 - 1 wraps to 0xFFFF (65535)  → R5 = 65535
;   Negative:   MOVI with -1 stores 0xFFFF     → R6 = 65535
;   Zero shift: SHL by 0 is identity           → R7 = 42
; ============================================================================

; --- Overflow test ---
MOVI R1, 255       ; R1 = 255 (0x00FF)
MOVI R2, 1         ; R2 = 1
SHL  R1, R1, 8     ; R1 = 255 << 8 = 0xFF00
MOVI R7, 255       ; R7 = 0x00FF
OR   R1, R1, R7    ; R1 = 0xFF00 | 0x00FF = 0xFFFF
ADD  R3, R1, R2    ; R3 = 0xFFFF + 1 = 0x0000 (overflow wraps!)

; --- Underflow test ---
MOVI R4, 0         ; R4 = 0
SUB  R5, R4, R2    ; R5 = 0 - 1 = 0xFFFF (underflow wraps!)

; --- Negative immediate ---
MOVI R6, -1        ; R6 = 0xFFFF (sign-extended from 9-bit)

; --- Zero shift ---
MOVI R7, 42        ; R7 = 42
SHL  R7, R7, 0     ; R7 = 42 << 0 = 42 (unchanged)

HALT
