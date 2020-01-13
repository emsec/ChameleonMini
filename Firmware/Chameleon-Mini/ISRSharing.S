; This file allows to share ISR at runtime with a low overhead.
; Functions that have to be shared MUST be defined as ISR_SHARED types
; (Defined in Common.h) to instruct GCC to compile them preserving the stack
;
;  Created on: 2019-12-06
;      Author: ceres-c
;

#include "Codec/Codec.h"
#include <avr/interrupt.h>

; Use this macro to call isr functions
.macro call_isr isr_address
push r30
push r31
lds r30, \isr_address
lds r31, \isr_address+1
icall
pop r31
pop r30
reti
.endm

; Find first pause and start sampling
.global CODEC_DEMOD_IN_INT0_VECT
CODEC_DEMOD_IN_INT0_VECT:
    call_isr isr_func_CODEC_DEMOD_IN_INT0_VECT

; Frame Delay Time PCD to PICC ends
.global CODEC_TIMER_SAMPLING_CCC_VECT
CODEC_TIMER_SAMPLING_CCC_VECT:
    call_isr isr_func_TCD0_CCC_vect
