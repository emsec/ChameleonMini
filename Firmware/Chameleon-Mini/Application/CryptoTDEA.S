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



; Input:
;     R17:R16 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of enciphering, LSB in R0
_EncryptDEA:
	; Reload Z with the key block pointer
	movw    r30, r16
    ; Encipher
    clh
    rjmp    _LoadKeyAndRunDEA


; Input:
;     R17:R16 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of enciphering, LSB in R0
_DecryptDEA:
	; Reload Z with the key block pointer
	movw    r30, r16
    ; Encipher
    seh
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
    ; Decipher
    seh
    rjmp    _LoadKeyAndRunDEA

;
; Common prologue and epilogue code
;

_CommonEpilogue:
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

;
; Triple DEA ECB Routines
;

; This routine performs Triple DEA encryption using keying option 2: K1, K2, K1.
;
; Input:
;     R25:R24 - Pointer to plaintext output buffer
;     R23:R22 - Pointer to ciphertext input buffer
;     R21:R20 - Key block pointer.
;
; Returns:
;     Nothing.
.global CryptoDecrypt2KTDEA
CryptoDecrypt2KTDEA:
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
    push    r16
    push    r17

    ; Load the plaintext pointer to Z and fetch data
    movw    r30, r22
    ld      r7, Z+
    ld      r6, Z+
    ld      r5, Z+
    ld      r4, Z+
    ld      r3, Z+
    ld      r2, Z+
    ld      r1, Z+
    ld      r0, Z+
    ; Encrypt
    movw    r16, r20
    rcall   _Decrypt2KTDEA
    ; Store the ciphertext
    movw    r30, r24
    st      Z+, r7
    st      Z+, r6
    st      Z+, r5
    st      Z+, r4
    st      Z+, r3
    st      Z+, r2
    st      Z+, r1
    st      Z+, r0

    ; Restore clobbered regs
    pop     r17
    pop     r16
    ; Reuse epilogue code
    rjmp    _CommonEpilogue


; This routine performs Triple DEA encryption using keying option 2: K1, K2, K1.
;
; Input:
;     R25:R24 - Pointer to plaintext input buffer
;     R23:R22 - Pointer to ciphertext output buffer
;     R21:R20 - Key block pointer.
;
; Returns:
;     Nothing.
.global CryptoEncrypt2KTDEA
CryptoEncrypt2KTDEA:
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
    push    r16
    push    r17

    ; Load the plaintext pointer to Z and fetch data
    movw    r30, r24
    ld      r7, Z+
    ld      r6, Z+
    ld      r5, Z+
    ld      r4, Z+
    ld      r3, Z+
    ld      r2, Z+
    ld      r1, Z+
    ld      r0, Z+
    ; Encrypt
    movw    r16, r20
    rcall   _Encrypt2KTDEA
    ; Store the ciphertext
    movw    r30, r22
    st      Z+, r7
    st      Z+, r6
    st      Z+, r5
    st      Z+, r4
    st      Z+, r3
    st      Z+, r2
    st      Z+, r1
    st      Z+, r0

    ; Restore clobbered regs
    pop     r17
    pop     r16
    ; Reuse epilogue code
    rjmp    _CommonEpilogue

;
; Triple DEA CBC Routines
;

; This routine performs the CBC "send" mode chaining: C = E(P ^ IV); IV = C
;
; Input:
;     R31:R30 - Cryptographic primitive pointer
;     R25:R24 - Count of blocks.
;     R23:R22 - Pointer to plaintext input buffer
;     R21:R20 - Pointer to ciphertext output buffer
;     R19:R18 - IV block pointer.
;     R17:R16 - Key block pointer.
;
; Returns:
;     Nothing.
;
_DEACBCSend:
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

    ; Load the plaintext pointer to Y
    movw    r28, r22
    ; Load the ciphertext pointer to X
    movw    r26, r20
    ; Store the crypto primitive pointer in r23:r22
    movw    r22, r30

    ; Load the IV pointer to Z
    movw    r30, r18
    ; Load the IV
    ld      r7, Z+
    ld      r6, Z+
    ld      r5, Z+
    ld      r4, Z+
    ld      r3, Z+
    ld      r2, Z+
    ld      r1, Z+
    ld      r0, Z+

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

    ; Call the primitive
    movw    r30, r22
    icall

    ; Store the ciphertext
    ; It will be reused as the IV for the next block, if any
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
    ; Store the updated IV
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
    rjmp    _CommonEpilogue

; This routine performs the CBC "receive" mode chaining: C = E(P) ^ IV; IV = P
;
; Input:
;     R31:R30 - Cryptographic primitive pointer
;     R25:R24 - Count of blocks.
;     R23:R22 - Pointer to plaintext input buffer
;     R21:R20 - Pointer to ciphertext output buffer
;     R19:R18 - IV block pointer.
;     R17:R16 - Key block pointer.
;
; Returns:
;     Nothing.
;
_DEACBCReceive:
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

    ; Load the plaintext pointer to Y
    movw    r28, r22
    ; Load the ciphertext pointer to X
    movw    r26, r20
    ; Store the crypto primitive pointer in r23:r22
    movw    r22, r30

1:
    ; Load the plaintext block
    ldd     r7, Y+0
    ldd     r6, Y+1
    ldd     r5, Y+2
    ldd     r4, Y+3
    ldd     r3, Y+4
    ldd     r2, Y+5
    ldd     r1, Y+6
    ldd     r0, Y+7

    ; Call the primitive
    movw    r30, r22
    icall
    
    ; Load the IV block
    movw    r30, r18
    ld      r15, Z+
    ld      r14, Z+
    ld      r13, Z+
    ld      r12, Z+
    ld      r11, Z+
    ld      r10, Z+
    ld      r9, Z+
    ld      r8, Z+
    ; XOR the ciphertext with the IV
    eor     r7, r15
    eor     r6, r14
    eor     r5, r13
    eor     r4, r12
    eor     r3, r11
    eor     r2, r10
    eor     r1, r9
    eor     r0, r8
    ; Reload the plaintext block
    ld      r15, Y+
    ld      r14, Y+
    ld      r13, Y+
    ld      r12, Y+
    ld      r11, Y+
    ld      r10, Y+
    ld      r9, Y+
    ld      r8, Y+
    ; Store the new IV before it gets potentially overwritten
    movw    r30, r18
    st      Z+, r15
    st      Z+, r14
    st      Z+, r13
    st      Z+, r12
    st      Z+, r11
    st      Z+, r10
    st      Z+, r9
    st      Z+, r8
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

    ; Restore clobbered regs
    pop     r29
    pop     r28
    rjmp    _CommonEpilogue


; This routine performs Triple DEA encryption in CBC mode using keying option 2: K1, K2, K1.
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
.global CryptoEncrypt2KTDEA_CBCSend
CryptoEncrypt2KTDEA_CBCSend:
    ldi     r31, pm_hi8(_Encrypt2KTDEA)
    ldi     r30, pm_lo8(_Encrypt2KTDEA)
    rjmp    _DEACBCSend

; This routine performs Triple DEA encryption in CBC mode using keying option 2: K1, K2, K1.
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
.global CryptoEncrypt2KTDEA_CBCReceive
CryptoEncrypt2KTDEA_CBCReceive:
    ldi     r31, pm_hi8(_Encrypt2KTDEA)
    ldi     r30, pm_lo8(_Encrypt2KTDEA)
    rjmp    _DEACBCReceive

; This routine performs Triple DEA decryption in CBC mode using keying option 2: K1, K2, K1.
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
.global CryptoDecrypt2KTDEA_CBCSend
CryptoDecrypt2KTDEA_CBCSend:
    ldi     r31, pm_hi8(_Decrypt2KTDEA)
    ldi     r30, pm_lo8(_Decrypt2KTDEA)
    rjmp    _DEACBCSend

; This routine performs Triple DEA decryption in CBC mode using keying option 2: K1, K2, K1.
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
.global CryptoDecrypt2KTDEA_CBCReceive
CryptoDecrypt2KTDEA_CBCReceive:
    ldi     r31, pm_hi8(_Decrypt2KTDEA)
    ldi     r30, pm_lo8(_Decrypt2KTDEA)
    rjmp    _DEACBCReceive

; This routine performs Triple DEA encryption in CBC mode using keying option 1: K1, K2, K3.
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
.global CryptoEncrypt3KTDEA_CBCSend
CryptoEncrypt3KTDEA_CBCSend:
    ldi     r31, pm_hi8(_Encrypt3KTDEA)
    ldi     r30, pm_lo8(_Encrypt3KTDEA)
    rjmp    _DEACBCSend

; This routine performs Triple DEA decryption in CBC mode using keying option 1: K1, K2, K3.
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
.global CryptoEncrypt3KTDEA_CBCReceive
CryptoEncrypt3KTDEA_CBCReceive:
    ldi     r31, pm_hi8(_Decrypt3KTDEA)
    ldi     r30, pm_lo8(_Decrypt3KTDEA)
    rjmp    _DEACBCReceive
