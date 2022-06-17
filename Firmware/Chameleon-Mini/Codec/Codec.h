/*
 * CODEC.h
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#ifndef CODEC_H_
#define CODEC_H_

/* Peripheral definitions */
#define CODEC_DEMOD_POWER_PORT      PORTB
#define CODEC_DEMOD_POWER_MASK      PIN0_bm
#define CODEC_DEMOD_IN_PORT         PORTB
#define CODEC_DEMOD_IN_MASK         (CODEC_DEMOD_IN_MASK0 | CODEC_DEMOD_IN_MASK1)
#define CODEC_DEMOD_IN_MASK0        PIN1_bm
#define CODEC_DEMOD_IN_MASK1        PIN2_bm
#define CODEC_DEMOD_IN_PINCTRL0     PIN1CTRL
#define CODEC_DEMOD_IN_PINCTRL1     PIN2CTRL
#define CODEC_DEMOD_IN_EVMUX0       EVSYS_CHMUX_PORTB_PIN1_gc
#define CODEC_DEMOD_IN_EVMUX1       EVSYS_CHMUX_PORTB_PIN2_gc
#define CODEC_DEMOD_IN_INT0_VECT	  PORTB_INT0_vect
#define CODEC_DEMOD_IN_INT1_VECT    PORTB_INT1_vect
#define CODEC_LOADMOD_PORT			PORTC
#define CODEC_LOADMOD_MASK			PIN6_bm
#define CODEC_CARRIER_IN_PORT		PORTC
#define CODEC_CARRIER_IN_MASK		PIN2_bm
#define CODEC_CARRIER_IN_PINCTRL 	PIN2CTRL
#define CODEC_CARRIER_IN_EVMUX		EVSYS_CHMUX_PORTC_PIN2_gc
#define CODEC_CARRIER_IN_DIV		2 /* external clock division factor */
#define CODEC_SUBCARRIER_PORT		PORTC
#define CODEC_SUBCARRIER_MASK_PSK	PIN4_bm
#define CODEC_SUBCARRIER_MASK_OOK	PIN5_bm
#define CODEC_SUBCARRIER_MASK		(CODEC_SUBCARRIER_MASK_PSK | CODEC_SUBCARRIER_MASK_OOK)
#define CODEC_SUBCARRIER_TIMER		TCC1
#define CODEC_SUBCARRIER_CC_PSK		CCA
#define CODEC_SUBCARRIER_CC_OOK		CCB
#define CODEC_SUBCARRIER_CCEN_PSK	TC1_CCAEN_bm
#define CODEC_SUBCARRIER_CCEN_OOK	TC1_CCBEN_bm
#define CODEC_TIMER_SAMPLING		TCD0
#define CODEC_TIMER_SAMPLING_OVF_VECT	TCD0_OVF_vect
#define CODEC_TIMER_SAMPLING_CCA_VECT	TCD0_CCA_vect
#define CODEC_TIMER_SAMPLING_CCB_VECT	TCD0_CCB_vect
#define CODEC_TIMER_SAMPLING_CCC_VECT   TCD0_CCC_vect
#define CODEC_TIMER_SAMPLING_CCD_VECT   TCD0_CCD_vect
#define CODEC_TIMER_LOADMOD       	TCE0
#define CODEC_TIMER_LOADMOD_OVF_VECT	TCE0_OVF_vect
#define CODEC_TIMER_LOADMOD_CCA_VECT	TCE0_CCA_vect
#define CODEC_TIMER_LOADMOD_CCB_VECT	TCE0_CCB_vect
#define CODEC_TIMER_LOADMOD_CCC_VECT	TCE0_CCC_vect
#define CODEC_TIMER_MODSTART_EVSEL	TC_EVSEL_CH0_gc
#define CODEC_TIMER_MODEND_EVSEL	TC_EVSEL_CH1_gc
#define CODEC_TIMER_CARRIER_CLKSEL	TC_CLKSEL_EVCH6_gc
#define CODEC_READER_TIMER			TCC0
#define CODEC_READER_PORT			PORTC
#define CODEC_READER_MASK_LEFT		PIN0_bm
#define CODEC_READER_MASK_RIGHT		PIN1_bm
#define CODEC_READER_MASK			(CODEC_READER_MASK_LEFT | CODEC_READER_MASK_RIGHT)
#define CODEC_READER_PINCTRL_LEFT	PIN0CTRL
#define CODEC_READER_PINCTRL_RIGHT	PIN1CTRL
#define CODEC_AC_DEMOD_SETTINGS		AC_HSMODE_bm | AC_HYSMODE_NO_gc
#define CODEC_MAXIMUM_THRESHOLD		0xFFF // the maximum voltage can be calculated with ch0data * Vref / 0xFFF
#define CODEC_THRESHOLD_CALIBRATE_MIN   128
#define CODEC_THRESHOLD_CALIBRATE_MID   768
#define CODEC_THRESHOLD_CALIBRATE_MAX   2048
#define CODEC_THRESHOLD_CALIBRATE_STEPS 16
#define CODEC_TIMER_TIMESTAMPS		TCD1
#define CODEC_TIMER_TIMESTAMPS_OVF_VECT	TCD1_OVF_vect
#define CODEC_TIMER_TIMESTAMPS_CCA_VECT	TCD1_CCA_vect
#define CODEC_TIMER_TIMESTAMPS_CCB_VECT	TCD1_CCB_vect

#ifndef __ASSEMBLER__

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "../Common.h"
#include "../Configuration.h"
#include "../Settings.h"

#include "ISO14443-2A.h"
#include "Reader14443-2A.h"
#include "SniffISO14443-2A.h"
#include "ISO15693.h"
#include "SniffISO15693.h"

/* Timing definitions for ISO14443A */
#define ISO14443A_SUBCARRIER_DIVIDER    16
#define ISO14443A_BIT_GRID_CYCLES       128
#define ISO14443A_BIT_RATE_CYCLES       128
#define ISO14443A_FRAME_DELAY_PREV1     1236
#define ISO14443A_FRAME_DELAY_PREV0     1172
#define ISO14443A_RX_PENDING_TIMEOUT	4 // ms

#define CODEC_BUFFER_SIZE           256

#define CODEC_CARRIER_FREQ          13560000

#define Codec8Reg0			GPIOR0
#define Codec8Reg1			GPIOR1
#define Codec8Reg2			GPIOR2
#define Codec8Reg3			GPIOR3
#define CodecCount16Register1		(*((volatile uint16_t*) &GPIOR4)) /* GPIOR4 & GPIOR5 */
#define CodecCount16Register2		(*((volatile uint16_t*) &GPIOR6)) /* GPIOR6 & GPIOR7 */
#define CodecPtrRegister1			(*((volatile uint8_t**) &GPIOR8))
#define CodecPtrRegister2			(*((volatile uint8_t**) &GPIORA))
#define CodecPtrRegister3			(*((volatile uint8_t**) &GPIORC))


extern uint16_t Reader_FWT;

#define FWI2FWT(x)	((uint32_t)(256 * 16 * ((uint32_t)1 << (x))) / (CODEC_CARRIER_FREQ / 1000) + 1)

typedef enum {
    CODEC_SUBCARRIERMOD_OFF,
    CODEC_SUBCARRIERMOD_OOK
} SubcarrierModType;

extern uint8_t CodecBuffer[CODEC_BUFFER_SIZE];
extern uint8_t CodecBuffer2[CODEC_BUFFER_SIZE];

extern enum RCTraffic {TRAFFIC_READER, TRAFFIC_CARD} SniffTrafficSource;

/* Shared ISR pointers and handlers */
extern void (* volatile isr_func_TCD0_CCC_vect)(void);
void isr_Reader14443_2A_TCD0_CCC_vect(void);
void isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT(void);
extern void (* volatile isr_func_CODEC_DEMOD_IN_INT0_VECT)(void);
void isr_ISO14443_2A_TCD0_CCC_vect(void);
void isr_ISO15693_CODEC_DEMOD_IN_INT0_VECT(void);
extern void (* volatile isr_func_CODEC_TIMER_LOADMOD_OVF_VECT)(void);
void isr_ISO14443_2A_CODEC_TIMER_LOADMOD_OVF_VECT(void);
void isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_OVF_VECT(void);
extern void (* volatile isr_func_CODEC_TIMER_LOADMOD_CCA_VECT)(void);
void isr_Reader14443_2A_CODEC_TIMER_LOADMOD_CCA_VECT(void);
void isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_CCA_VECT(void);
extern void (* volatile isr_func_CODEC_TIMER_LOADMOD_CCB_VECT)(void);
void isr_ISO15693_CODEC_TIMER_LOADMOD_CCB_VECT(void);
void isr_SniffISO14443_2A_CODEC_TIMER_LOADMOD_CCB_VECT(void);
void isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_CCA_VECT(void);
extern void (* volatile isr_func_CODEC_TIMER_TIMESTAMPS_CCA_VECT)(void);
void isr_Reader14443_2A_CODEC_TIMER_TIMESTAMPS_CCA_VECT(void);
void isr_SNIFF_ISO15693_CODEC_TIMER_TIMESTAMPS_CCA_VECT(void);
extern void (* volatile isr_func_ACA_AC0_vect)(void);
void isr_SniffISO14443_2A_ACA_AC0_VECT(void);
void isr_SNIFF_ISO15693_ACA_AC0_VECT(void);

INLINE void CodecInit(void) {
    ActiveConfiguration.CodecInitFunc();
}

INLINE void CodecDeInit(void) {
    ActiveConfiguration.CodecDeInitFunc();
}

INLINE void CodecTask(void) {
    ActiveConfiguration.CodecTaskFunc();
}

/* Helper Functions for Codec implementations */
INLINE void CodecInitCommon(void) {
    /* Configure CARRIER input pin and route it to EVSYS.
     * Multiply by 2 again by using both edges when externally
     * dividing by 2 */
#if CODEC_CARRIER_IN_DIV == 2
    CODEC_CARRIER_IN_PORT.CODEC_CARRIER_IN_PINCTRL = PORT_ISC_BOTHEDGES_gc;
#else
#error Option not supported
#endif
    CODEC_CARRIER_IN_PORT.DIRCLR = CODEC_CARRIER_IN_MASK;
    EVSYS.CH6MUX = CODEC_CARRIER_IN_EVMUX;

    /* Configure two DEMOD pins for input.
     * Configure event channel 0 for rising edge (begin of modulation pause)
     * Configure event channel 1 for falling edge (end of modulation pause) */
    CODEC_DEMOD_POWER_PORT.OUTCLR = CODEC_DEMOD_POWER_MASK;
    CODEC_DEMOD_POWER_PORT.DIRSET = CODEC_DEMOD_POWER_MASK;
    CODEC_DEMOD_IN_PORT.DIRCLR = CODEC_DEMOD_IN_MASK;
    CODEC_DEMOD_IN_PORT.CODEC_DEMOD_IN_PINCTRL0 = PORT_ISC_RISING_gc;
    CODEC_DEMOD_IN_PORT.CODEC_DEMOD_IN_PINCTRL1 = PORT_ISC_FALLING_gc;
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
    CODEC_DEMOD_IN_PORT.INT1MASK = 0;
    CODEC_DEMOD_IN_PORT.INTCTRL = PORT_INT0LVL_HI_gc | PORT_INT1LVL_HI_gc;
    EVSYS.CH0MUX = CODEC_DEMOD_IN_EVMUX0;
    EVSYS.CH1MUX = CODEC_DEMOD_IN_EVMUX1;

    /* Configure loadmod pin configuration and use a virtual port configuration
     * for single instruction cycle access */
    CODEC_LOADMOD_PORT.DIRSET = CODEC_LOADMOD_MASK;
    CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
    PORTCFG.VPCTRLA &= ~PORTCFG_VP0MAP_gm;
    PORTCFG.VPCTRLA |= PORTCFG_VP02MAP_PORTC_gc;

    /* Configure subcarrier pins for output */
    CODEC_SUBCARRIER_PORT.DIRSET = CODEC_SUBCARRIER_MASK;
    CODEC_SUBCARRIER_PORT.OUTCLR = CODEC_SUBCARRIER_MASK;

    /* Configure pins for reader field with the LEFT output being inverted
     * and all bridge outputs static high */
    CODEC_READER_PORT.CODEC_READER_PINCTRL_LEFT = PORT_INVEN_bm;
    CODEC_READER_PORT.OUTCLR = CODEC_READER_MASK_LEFT;
    CODEC_READER_PORT.OUTSET = CODEC_READER_MASK_RIGHT;
    CODEC_READER_PORT.DIRSET = CODEC_READER_MASK;

    /* Configure timer for generating reader field and configure AWEX for outputting pattern
     * with disabled outputs. */
    CODEC_READER_TIMER.CTRLB = TC0_CCAEN_bm | TC_WGMODE_SINGLESLOPE_gc;
    CODEC_READER_TIMER.PER = F_CPU / CODEC_CARRIER_FREQ - 1;
    CODEC_READER_TIMER.CCA = F_CPU / CODEC_CARRIER_FREQ / 2 ;

    AWEXC.OUTOVEN = 0x00;
    AWEXC.CTRL = AWEX_CWCM_bm | AWEX_DTICCAEN_bm | AWEX_DTICCBEN_bm;

    /* Configure DAC for the reference voltage */
    DACB.EVCTRL = 0;
    DACB.CTRLB = DAC_CHSEL_SINGLE_gc;
    DACB.CTRLC = DAC_REFSEL_AVCC_gc;
    DACB.CTRLA = DAC_IDOEN_bm | DAC_ENABLE_bm;
    DACB.CH0DATA = GlobalSettings.ActiveSettingPtr->ReaderThreshold; // real threshold voltage can be calculated with ch0data * Vref / 0xFFF

    /* Configure Analog Comparator 0 to detect changes in demodulated reader field */
    ACA.AC0MUXCTRL = AC_MUXPOS_DAC_gc | AC_MUXNEG_PIN7_gc;
    ACA.AC0CTRL = CODEC_AC_DEMOD_SETTINGS;

    /* Configure Analog Comparator 1 to detect SOC */
    ACA.AC1MUXCTRL = AC_MUXPOS_DAC_gc | AC_MUXNEG_PIN7_gc;
    ACA.AC1CTRL = CODEC_AC_DEMOD_SETTINGS;
}

INLINE void CodecSetSubcarrier(SubcarrierModType ModType, uint16_t Divider) {
    if (ModType == CODEC_SUBCARRIERMOD_OFF) {
        CODEC_SUBCARRIER_TIMER.CTRLA = TC_CLKSEL_OFF_gc;
        CODEC_SUBCARRIER_TIMER.CTRLB = 0;
    } else if (ModType == CODEC_SUBCARRIERMOD_OOK) {
        /* Configure subcarrier generation with 50% DC output using OOK */
        CODEC_SUBCARRIER_TIMER.CNT = 0;
        CODEC_SUBCARRIER_TIMER.PER = Divider - 1;
        CODEC_SUBCARRIER_TIMER.CODEC_SUBCARRIER_CC_OOK = Divider / 2;
        CODEC_SUBCARRIER_TIMER.CTRLB = CODEC_SUBCARRIER_CCEN_OOK | TC_WGMODE_SINGLESLOPE_gc;
    }
}

INLINE void CodecChangeDivider(uint16_t Divider) {
    CODEC_SUBCARRIER_TIMER.PER = Divider - 1;
}

INLINE void CodecStartSubcarrier(void) {
    CODEC_SUBCARRIER_TIMER.CTRLA = CODEC_TIMER_CARRIER_CLKSEL;
}

INLINE void CodecSetDemodPower(bool bOnOff) {
    if (bOnOff) {
        CODEC_DEMOD_POWER_PORT.OUTSET = CODEC_DEMOD_POWER_MASK;
    } else {
        CODEC_DEMOD_POWER_PORT.OUTCLR = CODEC_DEMOD_POWER_MASK;
    }
}

INLINE bool CodecGetLoadmodState(void) {
    if (ACA.STATUS & AC_AC0STATE_bm) {
        return true;
    } else {
        return false;
    }
}

INLINE void CodecSetLoadmodState(bool bOnOff) {
    if (bOnOff) {
        VPORT0.OUT |= CODEC_LOADMOD_MASK;
    } else {
        VPORT0.OUT &= ~CODEC_LOADMOD_MASK;
    }
}

// Turn on and off the codec Reader field
INLINE void CodecSetReaderField(bool bOnOff) { // this is the function for turning on/off the reader field dumbly; before using this function, please consider to use CodecReaderField{Start,Stop}

    if (bOnOff) {
        /* Start timer for field generation and unmask outputs */
        CODEC_READER_TIMER.CTRLA = TC_CLKSEL_DIV1_gc;
        AWEXC.OUTOVEN = CODEC_READER_MASK;
    } else {
        /* Disable outputs of AWEX and stop field generation */
        AWEXC.OUTOVEN = 0x00;
        CODEC_READER_TIMER.CTRLA = TC_CLKSEL_OFF_gc;
    }
}

// Get the status of the reader field
INLINE bool CodecGetReaderField(void) {
    return (CODEC_READER_TIMER.CTRLA == TC_CLKSEL_DIV1_gc) && (AWEXC.OUTOVEN == CODEC_READER_MASK);
}

void CodecReaderFieldStart(void);
void CodecReaderFieldStop(void);
bool CodecIsReaderFieldReady(void);

void CodecReaderFieldRestart(uint16_t delay);
#define FIELD_RESTART()	CodecReaderFieldRestart(100)
bool CodecIsReaderToBeRestarted(void);

void CodecThresholdSet(uint16_t th);
uint16_t CodecThresholdIncrement(void);
void CodecThresholdReset(void);

#endif /* __ASSEMBLER__ */

#endif /* CODEC_H_ */
