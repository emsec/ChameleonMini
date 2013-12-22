/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
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
#define CODEC_DEMOD_IN_MASK0        PIN0_bm
#define CODEC_DEMOD_IN_MASK1        PIN2_bm
#define CODEC_DEMOD_IN_PINCTRL0     PIN0CTRL
#define CODEC_DEMOD_IN_PINCTRL1     PIN2CTRL
#define CODEC_DEMOD_IN_EVMUX0       EVSYS_CHMUX_PORTB_PIN0_gc
#define CODEC_DEMOD_IN_EVMUX1       EVSYS_CHMUX_PORTB_PIN2_gc
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
