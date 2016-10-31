;
; DEA related code is kept in this file.
; All data is handled big-endian style (MSByte first) in memory.
;

.section .text

; This routine loads the key and performs 16 rounds of DEA.
;
; Input:
;     R31:R30 - A pointer to the 8-byte key (with parity bits), MSB first.
;     R7:R0   - 8-byte input data block, LSB in R0.
;     SREG:H  - Set to decipher, clear to encipher.
;
; Returns:
;     R7:R0   - Result of en/deciphering, LSB in R0
_LoadKeyAndRunDEA:
    ld      r15, Z+
    ld      r14, Z+
    ld      r13, Z+
    ld      r12, Z+
    ld      r11, Z+
    ld      r10, Z+
    ld      r9,  Z+
    ld      r8,  Z+
    des     0
    des     1
    des     2
    des     3
    des     4
    des     5
    des     6
    des     7
    des     8
    des     9
    des     10
    des     11
    des     12
    des     13
    des     14
    des     15
    ret

;
; Triple DEA subroutines
;

; This routine performs Triple DEA encryption (E-D-E) using keying option 1: K1, K2, K3.
;
; Input:
;     R17:R16 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of enciphering, LSB in R0
_Encrypt3KTDEA:
    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Encipher
    clh
    rcall   _LoadKeyAndRunDEA

    ; Z now points to K2
    ; Decipher
    seh
    rcall   _LoadKeyAndRunDEA

    ; Z now points to K3
    ; Encipher
    clh
    rjmp    _LoadKeyAndRunDEA

; This routine performs Triple DEA encryption (E-D-E) using keying option 2: K1, K2, K1.
;
; Input:
;     R17:R16 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of enciphering, LSB in R0
_Encrypt2KTDEA:
    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Encipher
    clh
    rcall   _LoadKeyAndRunDEA

    ; Z now points to K2
    ; Decipher
    seh
    rcall   _LoadKeyAndRunDEA

    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Encipher
    clh
    rjmp    _LoadKeyAndRunDEA


; This routine performs Triple DEA decryption (D-E-D) using keying option 1: K1, K2, K3.
;
; Input:
;     R17:R16 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of enciphering, LSB in R0
_Decrypt3KTDEA:
    ; Reload Z with the key block pointer and adjust to point to K3
    movw    r30, r16
    adiw    r30, 16
    ; Decipher
    seh
    rcall   _LoadKeyAndRunDEA

    ; Reload Z with the key block pointer and adjust to point to K2
    movw    r30, r16
    adiw    r30, 8
    ; Encipher
    clh
    rcall   _LoadKeyAndRunDEA

    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Decipher
    seh
    rjmp    _LoadKeyAndRunDEA

; This routine performs Triple DEA decryption (D-E-D) using keying option 2: K1, K2, K1.
;
; Input:
;     R17:R16 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of enciphering, LSB in R0
_Decrypt2KTDEA:
    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Decipher
    seh
    rcall   _LoadKeyAndRunDEA

    ; Z now points to K2
    ; Encipher
    clh
    rcall   _LoadKeyAndRunDEA

    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Encipher
    clh
    rjmp    _LoadKeyAndRunDEA

;
; Triple DES CBC Routines
;

; This routine performs Triple DES encryption in CBC mode using keying option 2: K1, K2, K1.
; The CBC is operated in the "send" mode: C = E(P ^ IV); IV = C
;
; Input:
;     R25:R24 - Count of blocks.
;     R23:R22 - Pointer to plaintext input buffer
;     R21:R20 - Pointer to ciphertext output buffer
;     R19:R18 - IV block pointer.
;     R17:R16 - Key block pointer.
;
; Returns:
;     Nothing.
.global CryptoEncrypt_2KTDEA_CBC_Send
CryptoEncrypt_2KTDEA_CBC_Send:
    ; Preserve the clobbered regs
    push    r2
    push    r3
    push    r4
    push    r5
    push    r6
    push    r7
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    push    r28
    push    r29

    ; Load the IV pointer to Y
    movw    r28, r18
    ; Load the IV
    ld      r15, Y+
    ld      r14, Y+
    ld      r13, Y+
    ld      r12, Y+
    ld      r11, Y+
    ld      r10, Y+
    ld      r9, Y+
    ld      r8, Y+

    ; Load the plaintext pointer to Y
    movw    r28, r22
    ; Load the ciphertext pointer to X
    movw    r26, r20

1:
    ; Load the plaintext block
    ld      r15, Y+
    ld      r14, Y+
    ld      r13, Y+
    ld      r12, Y+
    ld      r11, Y+
    ld      r10, Y+
    ld      r9, Y+
    ld      r8, Y+
    ; XOR the plaintext with the IV
    eor     r7, r15
    eor     r6, r14
    eor     r5, r13
    eor     r4, r12
    eor     r3, r11
    eor     r2, r10
    eor     r1, r9
    eor     r0, r8

    rcall   _Encrypt2KTDEA
    
    ; Store the ciphertext
    st      X+, r7
    st      X+, r6
    st      X+, r5
    st      X+, r4
    st      X+, r3
    st      X+, r2
    st      X+, r1
    st      X+, r0

    ; Decrement the counter, repeat if more blocks.
    sbiw    r24, 1
    brne    1b

    ; Load the IV pointer to X
    movw    r26, r18
    ; Store the IV
    st      X+, r7
    st      X+, r6
    st      X+, r5
    st      X+, r4
    st      X+, r3
    st      X+, r2
    st      X+, r1
    st      X+, r0

    ; Restore clobbered regs
    pop     r29
    pop     r28
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     r7
    pop     r6
    pop     r5
    pop     r4
    pop     r3
    pop     r2
    eor     r1, r1
    ret

; This routine performs Triple DES encryption in CBC mode using keying option 2: K1, K2, K1.
; The CBC is operated in the "receive" mode: C = E(P) ^ IV; IV = P
;
; Input:
;     R25:R24 - Count of blocks.
;     R23:R22 - Pointer to plaintext input buffer
;     R21:R20 - Pointer to ciphertext output buffer
;     R19:R18 - IV block pointer.
;     R17:R16 - Key block pointer.
;
; Returns:
;     Nothing.
.global CryptoEncrypt_2KTDEA_CBC_Receive
CryptoEncrypt_2KTDEA_CBC_Receive:
    ; Preserve the clobbered regs
    push    r2
    push    r3
    push    r4
    push    r5
    push    r6
    push    r7
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    push    r28
    push    r29

    ; Store the original IV pointer
    push    r18
    push    r19

    ; Load the plaintext pointer to Y
    movw    r28, r22
    ; Load the ciphertext pointer to X
    movw    r26, r20

1:
    ; Load the plaintext block
    ldd     r15, Y+0
    ldd     r14, Y+1
    ldd     r13, Y+2
    ldd     r12, Y+3
    ldd     r11, Y+4
    ldd     r10, Y+5
    ldd     r9, Y+6
    ldd     r8, Y+7

    rcall   _Encrypt2KTDEA
    
    ; Fetch the IV pointer
    movw    r30, r18
    ; Load the IV block
    ldd     r15, Z+0
    ldd     r14, Z+1
    ldd     r13, Z+2
    ldd     r12, Z+3
    ldd     r11, Z+4
    ldd     r10, Z+5
    ldd     r9, Z+6
    ldd     r8, Z+7
    ; XOR the ciphertext with the IV
    eor     r7, r15
    eor     r6, r14
    eor     r5, r13
    eor     r4, r12
    eor     r3, r11
    eor     r2, r10
    eor     r1, r9
    eor     r0, r8
    ; Update the IV pointer
    movw    r18, r28
    adiw    r28, 8
    ; Store the ciphertext
    st      X+, r7
    st      X+, r6
    st      X+, r5
    st      X+, r4
    st      X+, r3
    st      X+, r2
    st      X+, r1
    st      X+, r0

    ; Decrement the counter, repeat if more blocks.
    sbiw    r24, 1
    brne    1b

    ; Fetch the IV pointer
    movw    r30, r18
    ; Load the IV block pointer to X
    pop     r27
    pop     r26
    ; Copy over the IV
    ld      r0, Z+
    st      X+, r0
    ld      r0, Z+
    st      X+, r0
    ld      r0, Z+
    st      X+, r0
    ld      r0, Z+
    st      X+, r0
    ld      r0, Z+
    st      X+, r0
    ld      r0, Z+
    st      X+, r0
    ld      r0, Z+
    st      X+, r0
    ld      r0, Z+
    st      X+, r0

    ; Restore clobbered regs
    pop     r29
    pop     r28
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     r7
    pop     r6
    pop     r5
    pop     r4
    pop     r3
    pop     r2
    eor     r1, r1
    ret
