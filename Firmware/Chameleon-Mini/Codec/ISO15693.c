/*
 * ISO15693.c
 *
 *  Created on: 25.01.2017
 *      Author: Phillip Nash
 *      Modified by: ceres-c
 *
 *  Interrupts information
 *      - Compare/Capture Channels are used as compare. They're used to sample the field in different moments of time
 *      - Reader field demodulation events are routed on Event Channel 0, card data modulation on Event Channel 6
 *      - Reader field demodulation uses Compare Channel C, card data modulation Compare Channel D (PORTB in both cases, of course)
 *
 *  Loadmod code flow:
 *      Loadmodulation is performed via a state machine inside a ISR, triggered by a counter's (TCD0) overflow interrupt. The interrupt
 *      is configured in StartISO15693Demod to have a PER (period) equal to ISO15693 t1 nominal time: 4352 carrier pulses.
 *      (see ISO15693-3:2009, section 9.1).
 *      Once a EOF is reached when demodulating data from the reader, TCD0's PERBUF register is configured with the period of
 *      bit transmission (32 carrier pulses). This means that, once the first period overflow is reached (t1 expiration)
 *      the ISR will output data at the frequency dictated by the standard. This is due to the behaviour of buffered registers, see
 *      8045A-AVR-02/08 section 3.7.
 *
 *      Once t1 is reached (the counter overflowed), transmission of data should theoretically begin, but this is not guaranteed,
 *      due to possible slowdowns of data generation in the currently running Application. Until the application is done,
 *      the state machine will be stuck in LOADMOD_WAIT state, not outputting any data.
 *      The ISR will now be invoked every 32 carrier pulses (see ISO15693-2:2006, section 8.2), even when waiting for data.
 */

#include "ISO15693.h"
#include "../System.h"
#include "../Application/Application.h"
#include "LEDHook.h"
#include "AntennaLevel.h"
#include "Terminal/Terminal.h"
#include <avr/io.h>


#define SOC_1_OF_4_CODE         0x7B
#define SOC_1_OF_256_CODE       0x7E
#define EOC_CODE                0xDF
#define REQ_SUBCARRIER_SINGLE   0x00
#define REQ_SUBCARRIER_DUAL     0x01
#define REQ_DATARATE_LOW        0x00
#define REQ_DATARATE_HIGH       0x02
#define ISO15693_SAMPLE_CLK     TC_CLKSEL_DIV2_gc // 13.56MHz
#define ISO15693_SAMPLE_PERIOD  128 // 9.4us
#define ISO15693_T1_TIME        4352 - 14 /* ISO t1 time - ISR prologue compensation */ // 4192 + 128 + 128 - 1

#define SUBCARRIER_1            32
#define SUBCARRIER_2            28
#define SUBCARRIER_OFF          0
#define SOF_PATTERN             0x1D // 0001 1101
#define EOF_PATTERN             0xB8 // 1011 1000

// These registers provide quick access but are limited
// so global vars will be necessary
#define DataRegister            Codec8Reg0
#define StateRegister           Codec8Reg1
#define ModulationPauseCount    Codec8Reg2
#define SampleRegister          Codec8Reg3
#define BitSent                 CodecCount16Register1
#define BitSampleCount          CodecCount16Register2
#define CodecBufferPtr          CodecPtrRegister1

static volatile struct {
    volatile bool DemodFinished;
    volatile bool LoadmodFinished;
} Flags = { 0 };

typedef enum {
    DEMOD_SOC_STATE,
    DEMOD_1_OUT_OF_4_STATE,
    DEMOD_1_OUT_OF_256_STATE
} DemodStateType;

static volatile enum {
    LOADMOD_WAIT,

    /* Single subcarrier */
    LOADMOD_START_SINGLE,
    LOADMOD_SOF_SINGLE,
    LOADMOD_BIT0_SINGLE,
    LOADMOD_BIT1_SINGLE,
    LOADMOD_EOF_SINGLE,

    /* Dual subcarrier */
    LOADMOD_START_DUAL,
    LOADMOD_SOF_DUAL,
    LOADMOD_BIT0_DUAL,
    LOADMOD_BIT1_DUAL,
    LOADMOD_EOF_DUAL,

    LOADMOD_FINISHED
} LoadModState;

static volatile DemodStateType DemodState;
static volatile uint8_t ShiftRegister;
static volatile uint8_t ByteCount;
static volatile uint16_t BitRate1;
static volatile uint16_t BitRate2;
static volatile uint16_t SampleDataCount;

/* This function implements CODEC_DEMOD_IN_INT0_VECT interrupt vector.
 * It is called when a pulse is detected in CODEC_DEMOD_IN_PORT (PORTB).
 * The relevant interrupt vector was registered to CODEC_DEMOD_IN_MASK0 (PIN1) via:
 * CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
 * and unregistered writing the INT0MASK to 0
 */
ISR_SHARED isr_ISO15693_CODEC_DEMOD_IN_INT0_VECT(void) {
    /* Clear Compare Channel C (CCC) interrupt Flags - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;
    /* Enable compare/capture for high level interrupts on Capture Channel C for TCD0 - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_HI_gc;

    /* Disable this interrupt. From now on we will sample the field using our internal clock - From 13.13.11 [8331F–AVR–04/2013] */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;

}

/* This function is called from isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT
 * when we have 8 bits in SampleRegister and they represent an end of frame.
 */
INLINE void ISO15693_EOC(void) {
    /* Set bitrate required by the reader on SOF for our following response */
    if (CodecBuffer[0] & REQ_DATARATE_HIGH) {
        BitRate1 = 256;
        BitRate2 = 252; /* If single subcarrier mode is requested, BitRate2 is useless, but setting it nevertheless was still faster than checking */
    } else {
        BitRate1 = 256 * 4; // Possible value: 256 * 4 - 1
        BitRate2 = 252 * 4; // Possible value: 252 * 4 - 3
    }

    Flags.DemodFinished = 1;
    /* Disable demodulation interrupt */
    /* Sets timer off for TCD0, disabling clock source. We're done receiving data from reader and don't need to probe the antenna anymore - From 14.12.1 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    /* Disable event action for CODEC_TIMER_SAMPLING (TCD0) - From 14.12.4 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    /* Disable compare/capture interrupts on Channel C - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    /* Clear Compare Channel C (CCC) interrupt Flags - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;

    /* Enable loadmodulation interrupt */
    /* Disable the event action for TCE0 - From 14.12.4 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;
    /* Enable compare/capture for high level interrupts on channel B for TCE0 - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCBINTLVL_HI_gc;
    /* Clear TCE0 interrupt flags for Capture Channel B - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_CCBIF_bm;
    /* Set the BUFFERED PERIOD to Bitrate - 1 (because PERBUF is 0-based).
     * The current PERIOD was set in StartISO15693Demod to ISO15693_T1_TIME, once ISO15693_T1_TIME is reached, this will be the new PERIOD.
     * From 3.7 [8045A-AVR-02/08] */
    CODEC_TIMER_LOADMOD.PERBUF = BitRate1 - 1;
}

/* Disable data demodulation interrupt and inform the codec to restart demodulation from scratch */
INLINE void ISO15693_GARBAGE(void) {
    Flags.DemodFinished = 1;

    /* Sets timer off for TCD0, disabling clock source. We have demodulated rubbish and don't need to probe the antenna anymore - From 14.12.1 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    /* Disable event action for CODEC_TIMER_SAMPLING (TCD0) - From 14.12.4 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    /* Disable compare/capture interrupts on Channel C - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    /* Clear Compare Channel C (CCC) interrupt Flags - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;
}

/* This function is registered to CODEC_TIMER_SAMPLING (TCD0)'s Counter Channel C (CCC).
 * When the timer is enabled, this is called on counter's overflow
 *
 * It demodulates bits received from the reader and saves them in CodecBuffer.
 *
 * It disables its own interrupt when an EOF is found (calling ISO15693_EOC) or when it receives garbage
 */
ISR_SHARED isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT(void) {
    /* Shift demod data */
    SampleRegister = (SampleRegister << 1) | (!(CODEC_DEMOD_IN_PORT.IN & CODEC_DEMOD_IN_MASK) ? 0x01 : 0x00);

    if (++BitSampleCount == 8) {
        BitSampleCount = 0;
        switch (DemodState) {
            case DEMOD_SOC_STATE:
                if (SampleRegister == SOC_1_OF_4_CODE) {
                    DemodState = DEMOD_1_OUT_OF_4_STATE;
                    SampleDataCount = 0;
                    ModulationPauseCount = 0;
                } else if (SampleRegister == SOC_1_OF_256_CODE) {
                    DemodState = DEMOD_1_OUT_OF_256_STATE;
                    SampleDataCount = 0;
                } else { // No SOC. Restart and try again, we probably received garbage.
                    ISO15693_GARBAGE();
                }
                break;

            case DEMOD_1_OUT_OF_4_STATE:
                if (SampleRegister == EOC_CODE) {
                    ISO15693_EOC();
                } else {
                    uint8_t SampleData = ~SampleRegister;
                    if (SampleData == (0x01 << 6)) {
                        /* ^_^^^^^^ -> 00 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;

                    } else if (SampleData == (0x01 << 4)) {
                        /* ^^^_^^^^ -> 01 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;
                        DataRegister |= 0b01 << 6;

                    } else if (SampleData == (0x01 << 2)) {
                        /* ^^^^^_^^ -> 10 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;
                        DataRegister |= 0b10 << 6;

                    } else if (SampleData == (0x01 << 0)) {
                        /* ^^^^^^^_ -> 11 */
                        ModulationPauseCount++;
                        DataRegister >>= 2;
                        DataRegister |= 0b11 << 6;
                    }

                    if (ModulationPauseCount == 4) {
                        ModulationPauseCount = 0;
                        *CodecBufferPtr = DataRegister;
                        ++CodecBufferPtr;
                        ++ByteCount;
                    }
                }
                break;

            case DEMOD_1_OUT_OF_256_STATE:
                if (SampleRegister == EOC_CODE) {
                    ISO15693_EOC();
                } else {
                    uint8_t Position = ((SampleDataCount / 2) % 256) - 1;
                    uint8_t SampleData = ~SampleRegister;

                    if (SampleData == (0x01 << 6)) {
                        /* ^_^^^^^^ -> N-3 */
                        DataRegister = Position - 3;
                        ModulationPauseCount++;

                    } else if (SampleData == (0x01 << 4)) {
                        /* ^^^_^^^^ -> N-2 */
                        DataRegister = Position - 2;
                        ModulationPauseCount++;

                    } else if (SampleData == (0x01 << 2)) {
                        /* ^^^^^_^^ -> N-1 */
                        DataRegister = Position - 1;
                        ModulationPauseCount++;

                    } else if (SampleData == (0x01 << 0)) {
                        /* ^^^^^^^_ -> N-0 */
                        DataRegister = Position - 0;
                        ModulationPauseCount++;
                    }

                    if (ModulationPauseCount == 1) {
                        ModulationPauseCount = 0;
                        *CodecBufferPtr = DataRegister;
                        ++CodecBufferPtr;
                        ++ByteCount;
                    }
                }
                break;
        }
    }
    SampleDataCount++;
}

/* This function is registered to CODEC_TIMER_LOADMOD (TCE0)'s Counter Channel B (CCB).
 * When the timer is enabled, this is called on counter's overflow
 *
 * It modulates the carrier with consuming bytes in CodecBuffer until ByteCount is 0.
 *
 * It disables its own interrupt when all data has been sent
 */
ISR_SHARED isr_ISO15693_CODEC_TIMER_LOADMOD_CCB_VECT(void) {
    static void *JumpTable[] = {
        [LOADMOD_WAIT]          = && LOADMOD_WAIT_LABEL,
        [LOADMOD_START_SINGLE]  = && LOADMOD_START_SINGLE_LABEL,
        [LOADMOD_SOF_SINGLE]    = && LOADMOD_SOF_SINGLE_LABEL,
        [LOADMOD_BIT0_SINGLE]   = && LOADMOD_BIT0_SINGLE_LABEL,
        [LOADMOD_BIT1_SINGLE]   = && LOADMOD_BIT1_SINGLE_LABEL,
        [LOADMOD_EOF_SINGLE]    = && LOADMOD_EOF_SINGLE_LABEL,
        [LOADMOD_START_DUAL]    = && LOADMOD_START_DUAL_LABEL,
        [LOADMOD_SOF_DUAL]      = && LOADMOD_SOF_DUAL_LABEL,
        [LOADMOD_BIT0_DUAL]     = && LOADMOD_BIT0_DUAL_LABEL,
        [LOADMOD_BIT1_DUAL]     = && LOADMOD_BIT1_DUAL_LABEL,
        [LOADMOD_EOF_DUAL]      = && LOADMOD_EOF_DUAL_LABEL,
        [LOADMOD_FINISHED]      = && LOADMOD_FINISHED_LABEL
    };

    goto *JumpTable[StateRegister]; /* It takes 28 clock cycles to get here from function entry point */

LOADMOD_WAIT_LABEL:
    /* ISO t1 is over, but application is not done generating data yet */
    return;

LOADMOD_START_SINGLE_LABEL:
    /* Application produced data. With this interrupt we are aligned to the bit-grid. */
    ShiftRegister = SOF_PATTERN;
    BitSent = 0;
    /* Fallthrough */
LOADMOD_SOF_SINGLE_LABEL:
    if (ShiftRegister & 0x80) {
        CodecSetLoadmodState(true);
    } else {
        CodecSetLoadmodState(false);
    }

    CodecStartSubcarrier();

    ShiftRegister <<= 1;
    BitSent++;

    if ((BitSent % 8) == 0) {
        /* Last SOF bit has been put out. Start sending out data */
        StateRegister = LOADMOD_BIT0_SINGLE;
        ShiftRegister = (*CodecBufferPtr++);
    } else {
        StateRegister = LOADMOD_SOF_SINGLE;
    }
    return;

LOADMOD_BIT0_SINGLE_LABEL: //Manchester encoding
    if (ShiftRegister & 0x01) {
        /* Deactivate carrier */
        CodecSetLoadmodState(false);
    } else {
        /* Activate carrier */
        CodecSetLoadmodState(true);
    }

    StateRegister = LOADMOD_BIT1_SINGLE;
    return;

LOADMOD_BIT1_SINGLE_LABEL: //Manchester encoding
    if (ShiftRegister & 0x01) {
        CodecSetLoadmodState(true);
    } else {
        CodecSetLoadmodState(false);
    }

    ShiftRegister >>= 1;
    BitSent++;

    StateRegister = LOADMOD_BIT0_SINGLE;

    if ((BitSent % 8) == 0) {
        /* Byte boundary */
        if (--ByteCount == 0) {
            /* No more data left */
            ShiftRegister = EOF_PATTERN;
            StateRegister = LOADMOD_EOF_SINGLE;
        } else {
            ShiftRegister = (*CodecBufferPtr++);
        }
    }
    return;

LOADMOD_EOF_SINGLE_LABEL: //End of Manchester encoding
    /* Output EOF */
    if (ShiftRegister & 0x80) {
        CodecSetLoadmodState(true);
    } else {
        CodecSetLoadmodState(false);
    }

    ShiftRegister <<= 1;
    BitSent++;

    if ((BitSent % 8) == 0) {
        /* Last bit has been put out */
        StateRegister = LOADMOD_FINISHED;
    } /* else: stay in this state. No changes needed */
    return;

    // -------------------------------------------------------------

LOADMOD_START_DUAL_LABEL:
    ShiftRegister = SOF_PATTERN;
    BitSent = 0;
    CodecSetLoadmodState(true);
    CodecStartSubcarrier();
    // fallthrough
LOADMOD_SOF_DUAL_LABEL:
    if (ShiftRegister & 0x80) {
        CodecChangeDivider(SUBCARRIER_1);
        CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
    } else {
        CodecChangeDivider(SUBCARRIER_2);
        CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
    }

    ShiftRegister <<= 1;
    BitSent++;

    if ((BitSent % 8) == 0) {
        /* Last SOF bit has been put out. Start sending out data */
        StateRegister = LOADMOD_BIT0_DUAL;
        ShiftRegister = (*CodecBufferPtr++);
    } else {
        StateRegister = LOADMOD_SOF_DUAL;
    }
    return;

LOADMOD_BIT0_DUAL_LABEL: //Manchester encoding
    if (ShiftRegister & 0x01) {
        CodecChangeDivider(SUBCARRIER_2);
        CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
    } else {
        CodecChangeDivider(SUBCARRIER_1);
        CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
    }

    StateRegister = LOADMOD_BIT1_DUAL;
    return;

LOADMOD_BIT1_DUAL_LABEL: //Manchester encoding
    if (ShiftRegister & 0x01) {
        /* fc / 32 */
        CodecChangeDivider(SUBCARRIER_1);
        CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
    } else {
        /* fc / 28 */
        CodecChangeDivider(SUBCARRIER_2);
        CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
    }

    ShiftRegister >>= 1;
    BitSent++;

    StateRegister = LOADMOD_BIT0_DUAL;

    if ((BitSent % 8) == 0) {
        /* Byte boundary */
        if (--ByteCount == 0) {
            /* No more data left */
            ShiftRegister = EOF_PATTERN;
            StateRegister = LOADMOD_EOF_DUAL;
        } else {
            ShiftRegister = (*CodecBufferPtr++);
        }
    }
    return;

LOADMOD_EOF_DUAL_LABEL: //End of Manchester encoding
    /* Output EOF */
    if (ShiftRegister & 0x80) {
        CodecChangeDivider(SUBCARRIER_1);
        CODEC_TIMER_LOADMOD.PER = BitRate1 - 1;
    } else {
        CodecChangeDivider(SUBCARRIER_2);
        CODEC_TIMER_LOADMOD.PER = BitRate2 - 1;
    }

    ShiftRegister <<= 1;
    BitSent++;

    if ((BitSent % 8) == 0) {
        /* Last bit has been put out */
        StateRegister = LOADMOD_FINISHED;
    } /* else: stay in this state. No changes needed */
    return;

LOADMOD_FINISHED_LABEL:
    /* Sets timer off for CODEC_TIMER_LOADMOD (TCE0) disabling clock source as we're done modulating */
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
    /* Disable compare/capture for all level interrupts on channel B for TCE0 - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCBINTLVL_OFF_gc;
    CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OFF, 0);
    Flags.LoadmodFinished = 1;
    return;
}

/* Reset global variables/interrupts to demodulate incoming reader data via ISRs */
void StartISO15693Demod(void) {
    /* Reset global variables to default values */
    CodecBufferPtr = CodecBuffer;
    Flags.DemodFinished = 0;
    Flags.LoadmodFinished = 0;
    DemodState = DEMOD_SOC_STATE;
    StateRegister = LOADMOD_WAIT;
    DataRegister = 0;
    SampleRegister = 0;
    BitSampleCount = 0;
    SampleDataCount = 0;
    ModulationPauseCount = 0;
    ByteCount = 0;
    ShiftRegister = 0;

    /* Set clock source for TCD0 to ISO15693_SAMPLE_CLK = TC_CLKSEL_DIV2_gc = System Clock / 2.
     * Since the Chameleon is clocked at 13.56*2 MHz (see Makefile), this counter will hit at the same frequency of reader field.
     * From 14.12.1 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_SAMPLING.CTRLA = ISO15693_SAMPLE_CLK;
    /* Set up TCD0 action/source:
     *  - Event Action: Restart waveform period (reset PER register)
     *  - Event Source Select: trigger on CODEC_TIMER_MODSTART_EVSEL = TC_EVSEL_CH0_gc = Event Channel 0
     * From 14.12.4 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    /* Resets the sampling counter (TCD0) to 0 - From 14.12.12 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CNT = 0;

    /* Set Clock Source for TCE0 to CODEC_TIMER_CARRIER_CLKSEL = TC_CLKSEL_EVCH6_gc = Event Channel 6 */
    CODEC_TIMER_LOADMOD.CTRLA = CODEC_TIMER_CARRIER_CLKSEL;
    /* Set up TCE0 action/source:
     *  - Event Action: Restart waveform period (reset PER register)
     *  - Event Source Select: trigger on CODEC_TIMER_MODSTART_EVSEL = TC_EVSEL_CH0_gc = Event Channel 0
     * From 14.12.4 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    /* Temporarily disable compare/capture for all level interrupts on channel B for TCE0. They'll be enabled later when load modulation will be needed.
     * From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCBINTLVL_OFF_gc;
    /* Set the period for CODEC_TIMER_LOADMOD (TCE0) ISO standard t1 time.
     * Once this timer is enabled, it will wait for a time equal to t1 and then its ISR will be called.
     * From 14.12.18 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_LOADMOD.PER = ISO15693_T1_TIME;

    /* Start looking out for modulation pause via interrupt. */
    /* Clear PORTB interrupt flags - From 13.13.13 [8331F–AVR–04/2013] */
    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT0IF_bm;
    /* Use PIN1 as a source for modulation pauses. Will trigger PORTB Interrput 0 - From 13.13.11 [8331F–AVR–04/2013] */
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
}

/* Configure unvarying interrupts settings.
 * All configured interrupts will still be disabled once this function is done, they'll be enabled one by one as the communication flow requires them.
 */
void ISO15693CodecInit(void) {
    CodecInitCommon();

    /**************************************************
     *    Register function handlers to shared ISR    *
     **************************************************/
    /* Register isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT function to CODEC_TIMER_SAMPLING (TCD0)'s Counter Channel C (CCC) */
    isr_func_TCD0_CCC_vect = &isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT;
    /* Register isr_ISO15693_CODEC_DEMOD_IN_INT0_VECT function to CODEC_DEMOD_IN_PORT (PORTB) interrupt 0 */
    isr_func_CODEC_DEMOD_IN_INT0_VECT = &isr_ISO15693_CODEC_DEMOD_IN_INT0_VECT;
    /* Register isr_ISO15693_CODEC_TIMER_LOADMOD_CCB_VECT function to CODEC_TIMER_LOADMOD (TCE0)'s Counter Channel B (CCB) */
    isr_func_CODEC_TIMER_LOADMOD_CCB_VECT = &isr_ISO15693_CODEC_TIMER_LOADMOD_CCB_VECT;

    /**************************************************
     *           Configure demod interrupt            *
     **************************************************/
    /* Configure sampling-timer free running and sync to first modulation-pause */
    /* Set the period for TCD0 to ISO15693_SAMPLE_PERIOD - 1 because PER is 0-based - From 14.12.14 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.PER = ISO15693_SAMPLE_PERIOD - 1;
    /* Set Compare Channel C (CCC) period to half bit period - 1. (- 14 to compensate ISR timing overhead) - From 14.12.16 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CCC = ISO15693_SAMPLE_PERIOD / 2 - 14 - 1;
    /* Temporarily disable Compare Channel C (CCC) interrupts.
     * They'll be enabled later in isr_ISO15693_CODEC_DEMOD_IN_INT0_VECT once we sensed the reader is sending data and we're in sync with the pulses.
     * From 14.12.7 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    /* Clear Compare Channel C (CCC) interrupt Flags - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;

    /**************************************************
     *          Configure loadmod interrupt           *
     **************************************************/
    /* Disable timer error and overflow interrupts - From 14.12.6 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.INTCTRLA = 0;

    /* Activate Power for demodulator */
    CodecSetDemodPower(true);

    StartISO15693Demod();
}

void ISO15693CodecDeInit(void) {
    /* Gracefully shutdown codec */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;

    /* Reset global variables to default values */
    CodecBufferPtr = CodecBuffer;
    Flags.DemodFinished = 0;
    Flags.LoadmodFinished = 0;
    DemodState = DEMOD_SOC_STATE;
    StateRegister = LOADMOD_WAIT;
    DataRegister = 0;
    SampleRegister = 0;
    BitSampleCount = 0;
    SampleDataCount = 0;
    ModulationPauseCount = 0;
    ByteCount = 0;
    ShiftRegister = 0;

    /* Disable sample timer */
    /* Sets timer off for TCD0, disabling clock source - From 14.12.1 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    /* Disable event action for CODEC_TIMER_SAMPLING (TCD0) - From 14.12.4 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    /* Disable Compare Channel C (CCC) interrupts - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    /* Clear Compare Channel C (CCC) interrupt Flags - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;

    /* Disable load modulation */
    /* Disable the event action for TCE0 - From 14.12.4 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;
    /* Disable compare/capture for all level interrupts on channel B for TCE0 - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCBINTLVL_OFF_gc;
    /* Clear TCE0 interrupt flags for Capture Channel B - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_CCBIF_bm;

    CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OFF, 0);
    CodecSetDemodPower(false);
    CodecSetLoadmodState(false);
}

void ISO15693CodecTask(void) {
    if (Flags.DemodFinished) {
        Flags.DemodFinished = 0;

        uint16_t DemodByteCount = ByteCount;
        uint16_t AppReceivedByteCount = 0;
        bool bDualSubcarrier = false;

        if (DemodByteCount > 0) {
            LogEntry(LOG_INFO_CODEC_RX_DATA, CodecBuffer, DemodByteCount);
            LEDHook(LED_CODEC_RX, LED_PULSE);

            if (CodecBuffer[0] & REQ_SUBCARRIER_DUAL) {
                bDualSubcarrier = true;
            }
            AppReceivedByteCount = ApplicationProcess(CodecBuffer, DemodByteCount);
        }

        /* This is only reached when we've received a valid frame */
        if (AppReceivedByteCount != ISO15693_APP_NO_RESPONSE) {
            if (AppReceivedByteCount > CODEC_BUFFER_SIZE - ISO15693_CRC16_SIZE) { /* CRC would be written outside codec buffer */
                CodecBuffer[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                CodecBuffer[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_NOT_SUPP;
                AppReceivedByteCount = 2;
                LogEntry(LOG_INFO_GENERIC, "Too much data requested - See PR #274", 38);
            }

            LEDHook(LED_CODEC_TX, LED_PULSE);

            ByteCount = AppReceivedByteCount;
            CodecBufferPtr = CodecBuffer;

            /* Start loadmodulating */
            if (bDualSubcarrier) {
                CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OOK, SUBCARRIER_2);
                StateRegister = LOADMOD_START_DUAL;
            } else {
                CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OOK, SUBCARRIER_1);
                StateRegister = LOADMOD_START_SINGLE;
            }

            /* Calculate the CRC while modulation is already ongoing */
            ISO15693AppendCRC(CodecBuffer, AppReceivedByteCount);
            ByteCount += ISO15693_CRC16_SIZE; /* Increase this variable as it will be read by the codec during loadmodulation */
            LogEntry(LOG_INFO_CODEC_TX_DATA, CodecBuffer, AppReceivedByteCount + ISO15693_CRC16_SIZE);

        } else {
            /* Overwrite the PERBUF register, which was configured in ISO15693_EOC, with the new appropriate value.
             * This is expecially needed because, since load modulation has not been performed, we're jumping here straight after
             * ISO15693_EOC. This implies that the data stored in the PERBUF register by ISO15693_EOC still has to be copied
             * in the PER register and that will happen on the next UPDATE event.
             * The next UPDATE event will be timer/counter enablement in ISO15693_EOC, which is executed after StartISO15693Demod,
             * effectively overwriting PER with an incorrect value.
             * See 8045A-AVR-02/08 section 3.7 for information about buffered registers.
             */
            CODEC_TIMER_LOADMOD.PERBUF = ISO15693_T1_TIME;
            StartISO15693Demod();
        }
    }

    if (Flags.LoadmodFinished) {
        Flags.LoadmodFinished = 0;
        /* Load modulation has been finished. Stop it and start to listen for incoming data again. */
        StartISO15693Demod();
    }
}
