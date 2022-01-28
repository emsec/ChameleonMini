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
; For development purposes: cycles penality before landing in the Shared ISR is 10
.macro call_isr isr_address
push r30                    ; 1
push r31                    ; 1
lds r30, \isr_address       ; 3
lds r31, \isr_address+1     ; 3
icall                       ; 2 (16 bit PC)
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

.global CODEC_TIMER_LOADMOD_OVF_VECT
CODEC_TIMER_LOADMOD_OVF_VECT:
    call_isr isr_func_CODEC_TIMER_LOADMOD_OVF_VECT

.global CODEC_TIMER_LOADMOD_CCA_VECT
CODEC_TIMER_LOADMOD_CCA_VECT:
    call_isr isr_func_CODEC_TIMER_LOADMOD_CCA_VECT

.global CODEC_TIMER_LOADMOD_CCB_VECT
CODEC_TIMER_LOADMOD_CCB_VECT:
    call_isr isr_func_CODEC_TIMER_LOADMOD_CCB_VECT

.global CODEC_TIMER_TIMESTAMPS_CCA_VECT
CODEC_TIMER_TIMESTAMPS_CCA_VECT:
    call_isr isr_func_CODEC_TIMER_TIMESTAMPS_CCA_VECT

.global ACA_AC0_vect
ACA_AC0_vect:
    call_isr isr_func_ACA_AC0_vect
