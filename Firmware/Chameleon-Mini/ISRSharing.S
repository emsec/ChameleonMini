; This file allows to share ISR at runtime with a low overhead.
; Functions that have to be shared MUST be defined as ISR_SHARED types
; (Defined in Common.h) to instruct GCC to compile them preserving the stack
;
;  Created on: 2019-12-06
;      Author: ceres-c
;

#include "Codec/Codec.h"
#include <avr/interrupt.h>

; Macro to call ISR functions
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

; Shared ISR must be defined below as globals
; Example:
;
; .global <INTERRUPT-TO-SHARE>
; INTERRUPT-TO-SHARE:
;     call_isr POINTER-TO-VARIABLE-INTERRUPT-HANDLER
;
; Where:
; INTERRUPT-TO-SHARE
;   The target interrupt which needs to be shared
; POINTER-TO-VARIABLE-INTERRUPT-HANDLER
;   A pointer to the interrupt handler which can be modified at runtime.
;   Must be a volatile pointer to function declared in Codec.h

.global CODEC_DEMOD_IN_INT0_VECT
CODEC_DEMOD_IN_INT0_VECT:
    call_isr isr_func_CODEC_DEMOD_IN_INT0_VECT

.global CODEC_TIMER_SAMPLING_CCC_VECT
CODEC_TIMER_SAMPLING_CCC_VECT:
    call_isr isr_func_TCD0_CCC_vect

.global CODEC_TIMER_LOADMOD_CCB_VECT
CODEC_TIMER_LOADMOD_CCB_VECT:
    call_isr isr_func_CODEC_TIMER_LOADMOD_CCB_VECT