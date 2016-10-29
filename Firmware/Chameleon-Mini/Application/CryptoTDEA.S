
.section .text

; This routine performs 16 rounds of DES.
;
; Input:
;     R15:R8  - 8-byte key (with parity bits), LSB in R8.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of en/deciphering, LSB in R0
CryptoEncrypt_DEA:
    des 0
    des 1
    des 2
    des 3
    des 4
    des 5
    des 6
    des 7
    des 8
    des 9
    des 10
    des 11
    des 12
    des 13
    des 14
    des 15
    ret

; This routine performs Triple DES encryption using keying option 2: K1, K2, K1.
;
; Input:
;     R17:R16 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of enciphering, LSB in R0
CryptoEncrypt_2KTDEA:
    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Load the K1
    ld      r15, Z+
    ld      r14, Z+
    ld      r13, Z+
    ld      r12, Z+
    ld      r11, Z+
    ld      r10, Z+
    ld      r9,  Z+
    ld      r8,  Z+
    ; Encipher
    clh
    rcall   CryptoEncrypt_DEA
    ; Load the K2
    ld      r15, Z+
    ld      r14, Z+
    ld      r13, Z+
    ld      r12, Z+
    ld      r11, Z+
    ld      r10, Z+
    ld      r9,  Z+
    ld      r8,  Z+
    ; Decipher
    seh
    rcall   CryptoEncrypt_DEA
    ; Reload Z with the key block pointer
    movw    r30, r16
    ; Load the K1
    ld      r15, Z+
    ld      r14, Z+
    ld      r13, Z+
    ld      r12, Z+
    ld      r11, Z+
    ld      r10, Z+
    ld      r9,  Z+
    ld      r8,  Z+
    ; Encipher
    clh
    rjmp    CryptoEncrypt_DEA

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

CryptoEncrypt_2KTDEA_CBC_Send_loop:
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

    rcall   CryptoEncrypt_2KTDEA
    
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
    brne    CryptoEncrypt_2KTDEA_CBC_Send_loop

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

CryptoEncrypt_2KTDEA_CBC_Receive_loop:
    ; Load the plaintext block
    ldd     r15, Y+0
    ldd     r14, Y+1
    ldd     r13, Y+2
    ldd     r12, Y+3
    ldd     r11, Y+4
    ldd     r10, Y+5
    ldd     r9, Y+6
    ldd     r8, Y+7

    rcall   CryptoEncrypt_2KTDEA
    
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
    brne    CryptoEncrypt_2KTDEA_CBC_Receive_loop

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
