
.section .text

; This routine performs 16 rounds of DES.
;
; Input:
;     R15:R8  - 8-byte key (with parity bits), LSB in R8.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of en/deciphering, LSB in R0
do_des_rounds:
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
;     R21:R20 - Key block pointer.
;     R7:R0   - Input data, LSB in R0
;
; Returns:
;     R7:R0   - Result of en/deciphering, LSB in R0
encrypt_2ktdea:
    ; Reload Z with the key block pointer
    movw    r30, r18
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
    rcall   do_des_rounds
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
    rcall   do_des_rounds
    ; Reload Z with the key block pointer
    movw    r30, r18
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
    rjmp    do_des_rounds

;
; Triple DES CBC Routines
;

; This routine performs Triple DES encryption in CBC mode using keying option 2: K1, K2, K1.
; The IV is set to all zeros.
;
; Input:
;     R25:R24 - Count of blocks.
;     R23:R22 - Pointer to plaintext input buffer
;     R21:R20 - Pointer to ciphertext output buffer
;     R19:R18 - Key block pointer.
;
; Returns:
;     Nothing.
.global CryptoEncrypt_2KTDEA_CBC
CryptoEncrypt_2KTDEA_CBC:
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

    ; Set IV to all zeros
    mov     r0, r1
    movw    r2, r0
    movw    r4, r0
    movw    r6, r0

CryptoEncrypt_2KTDEA_CBC_loop:
    ; Load and XOR plaintext with the IV.
    ld      r31, Y+
    eor     r7, r31
    ld      r31, Y+
    eor     r6, r31
    ld      r31, Y+
    eor     r5, r31
    ld      r31, Y+
    eor     r4, r31
    ld      r31, Y+
    eor     r3, r31
    ld      r31, Y+
    eor     r2, r31
    ld      r31, Y+
    eor     r1, r31
    ld      r31, Y+
    eor     r0, r31

    rcall   encrypt_2ktdea
    
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
    brne    CryptoEncrypt_2KTDEA_CBC_loop

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
