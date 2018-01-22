#include <avr/io.h>

#define Zero            R0
#define Tmp             R16
#define BitCountL       R18
#define BitCountH       R19
#define NewLoad         R20
#define SampleRegister  R21

#define GPIORBitCountL  4
#define GPIORBitCountH  5
#define CodecBufferPtrL 10
#define CodecBufferPtrH 11

#define AWEXC__OUTOVEN              0x088C
#define CODEC_READER_TIMER__CTRLA   0x0800

.global TCD0_CCB_vect, Reader14443AMillerEOC
TCD0_CCB_vect:
push Zero                                                   ; 1
eor Zero, Zero                                              ; 1
push Tmp                                                    ; 1
push BitCountL                                              ; 1
push BitCountH                                              ; 1
push NewLoad                                                ; 1
push SampleRegister                                         ; 1
in Tmp, 0x3f ; SREG                                         ; 1
push Tmp                                                    ; 1
push ZL                                                     ; 1
push ZH                                                     ; 1
                                                            ; SUM: 8
in ZL, CodecBufferPtrL
in ZH, CodecBufferPtrH
ld SampleRegister, Z+
clr NewLoad

in BitCountH, GPIORBitCountH
in BitCountL, GPIORBitCountL



LOOP:
; POINT ZERO
lsr SampleRegister                                          ; 1
brcc NO_TURNOFF_COMPENSATION                                ; 1 / 2

    sts AWEXC__OUTOVEN, Zero ; turn off field               ; 2
    sts CODEC_READER_TIMER__CTRLA, Zero                     ; 2

    ldi Tmp, 0x16                                           ; 1
    TURNOFF_LOOP:
        dec Tmp                                             ; 1
    brne TURNOFF_LOOP                                       ; 1 / 2 ; sums up to 21 * 3 + 2
    rjmp .+0 ; double nop                                   ; 2

    ldi Tmp, 0x01                                           ; 1
    sts CODEC_READER_TIMER__CTRLA, Tmp  ; turn on field     ; 2
    ldi Tmp, 0x03                                           ; 1
    sts AWEXC__OUTOVEN, Tmp                                 ; 2

    rjmp NO_TURNOFF                                         ; 2
                                                            ; SUM: 82 until now


NO_TURNOFF_COMPENSATION:
ldi Tmp, 26                                                 ; 1
NO_TURNOFF_COMPENSATION_LOOP:
    dec Tmp                                                 ; 1
    brne NO_TURNOFF_COMPENSATION_LOOP                       ; 1 / 2 sums up to 25 * 3 + 2
nop                                                         ; 1
NO_TURNOFF:                                                 ; 82 at this point
subi BitCountL, 1 ; decrement BitCount                      ; 1
sbci BitCountH, 0                                           ; 1
brne NO_EOC                                                 ; 1 / 2

    ; EOC:
    call Reader14443AMillerEOC
    rjmp RETURN

NO_EOC:                                                     ; 86 at this point
subi NewLoad, 0xFF                                          ; 1
andi NewLoad, 0x07                                          ; 1
brne LOAD_COMPENSATION                                      ; 1 / 2
                                                            ; SUM: 4
    ld SampleRegister, Z+                                   ; 3
    rjmp NOP_LOOP_INIT                                      ; 2

LOAD_COMPENSATION: ; 90 at this point
rjmp .+0 ; double nop                                       ; 2
rjmp .+0 ; double nop                                       ; 2
NOP_LOOP_INIT:
ldi Tmp, 10                                                 ; 1
NOP_LOOP:
    dec Tmp                                                 ; 1
    brne NOP_LOOP                                           ; 1 / 2 sums up to 9 * 3 + 2
rjmp .+0                                                    ; 2
rjmp LOOP                                                   ; 2

RETURN:
pop ZH
pop ZL
pop Tmp
out 0x3f, Tmp ; SREG
pop SampleRegister
pop NewLoad
pop BitCountH
pop BitCountL
pop Tmp
pop Zero
reti                                                        ; 2
