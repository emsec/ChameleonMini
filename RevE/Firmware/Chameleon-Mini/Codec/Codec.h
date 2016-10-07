/*
 * CODEC.h
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#ifndef CODEC_H_
#define CODEC_H_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "../Common.h"
#include "../Configuration.h"

#include "ISO14443-2A.h"

#define CODEC_DEMOD_POWER_PORT      PORTB
#define CODEC_DEMOD_POWER_MASK      PIN1_bm
#define CODEC_DEMOD_IN_PORT         PORTB
#define CODEC_DEMOD_IN_MASK         (CODEC_DEMOD_IN_MASK0 | CODEC_DEMOD_IN_MASK1)
#define CODEC_DEMOD_IN_MASK0        PIN2_bm
#define CODEC_DEMOD_IN_MASK1        PIN0_bm
#define CODEC_DEMOD_IN_PINCTRL0     PIN2CTRL
#define CODEC_DEMOD_IN_PINCTRL1     PIN0CTRL
#define CODEC_DEMOD_IN_EVMUX0       EVSYS_CHMUX_PORTB_PIN2_gc
#define CODEC_DEMOD_IN_EVMUX1       EVSYS_CHMUX_PORTB_PIN0_gc
#define CODEC_DEMOD_IN_INT0_VECT	PORTB_INT0_vect
#define CODEC_LOADMOD_PORT			PORTC
#define CODEC_LOADMOD_MASK			PIN6_bm
#define CODEC_CARRIER_IN_PORT		PORTC
#define CODEC_CARRIER_IN_MASK		PIN2_bm
#define CODEC_CARRIER_IN_PINCTRL 	PIN2CTRL
#define CODEC_CARRIER_IN_EVMUX		EVSYS_CHMUX_PORTC_PIN2_gc
#define CODEC_SUBCARRIER_PORT		PORTC
#define CODEC_SUBCARRIER_MASK_PSK	PIN0_bm
#define CODEC_SUBCARRIER_MASK_OOK	PIN1_bm
#define CODEC_SUBCARRIER_MASK		(CODEC_SUBCARRIER_MASK_PSK | CODEC_SUBCARRIER_MASK_OOK)
#define CODEC_SUBCARRIER_TIMER		TCC0
#define CODEC_SUBCARRIER_CC_PSK		CCA
#define CODEC_SUBCARRIER_CC_OOK		CCB
#define CODEC_SUBCARRIER_CCEN_PSK	TC0_CCAEN_bm
#define CODEC_SUBCARRIER_CCEN_OOK	TC0_CCBEN_bm
#define CODEC_TIMER_SAMPLING		TCC1
#define CODEC_TIMER_SAMPLING_CCA_VECT	TCC1_CCA_vect
#define CODEC_TIMER_LOADMOD       	TCD1
#define CODEC_TIMER_OVF_VECT		TCD1_OVF_vect

#define CODEC_BUFFER_SIZE           256 /* Byte */

#define CODEC_CARRIER_FREQ          13560000

extern uint8_t CodecBuffer[CODEC_BUFFER_SIZE];

INLINE void CodecInit(void) {
    ActiveConfiguration.CodecInitFunc();
}

INLINE void CodecTask(void) {
    ActiveConfiguration.CodecTaskFunc();
}

INLINE void CodecSetDemodPower(bool bOnOff) {
    CODEC_DEMOD_POWER_PORT.DIRSET = CODEC_DEMOD_POWER_MASK;

    if (bOnOff) {
        CODEC_DEMOD_POWER_PORT.OUTSET = CODEC_DEMOD_POWER_MASK;
    } else {
        CODEC_DEMOD_POWER_PORT.OUTCLR = CODEC_DEMOD_POWER_MASK;
    }
}

#endif /* CODEC_H_ */
