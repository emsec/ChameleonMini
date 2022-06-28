/*
 * SniffISO15693.c
 *
 *  Created on: 25.01.2017
 *      Author: ceres-c
 */

#include "SniffISO15693.h"
#include "Codec.h"
#include "../System.h"
#include "../Application/Application.h"
#include "LEDHook.h"
#include "AntennaLevel.h"

#define VCD_SOC_1_OF_4_CODE         0x7B
#define VCD_SOC_1_OF_256_CODE       0x7E
#define VCD_EOC_CODE                0xDF
#define VICC_SOC_CODE               0b00011101 /* = 0x1D 3 unmodulated periods, 3 modulated periods, 1 unmodulated, 1 modulated */
#define VICC_EOC_CODE               0b10111000 /* = 0xB8 1 modulated period, 1 unmodulated period, 3 modulated, 3 unmodulated */

#define ISO15693_READER_SAMPLE_CLK      TC_CLKSEL_DIV2_gc // 13.56MHz
#define ISO15693_READER_SAMPLE_PERIOD   128 // 9.4us

// These registers provide quick access but are limited
// so global vars will be necessary
#define DataRegister            Codec8Reg0
#define StateRegister           Codec8Reg1
#define ModulationPauseCount    Codec8Reg2
#define BitSampleCount          Codec8Reg3 /* Store the amount of received half-bits */
#define SampleRegister          CodecCount16Register1
#define SampleRegisterH         GPIOR4 /* CodecCount16Register1 is the composition of GPIOR4 and GPIOR5, divide them for easier read access */
#define SampleRegisterL         GPIOR5
#define DemodFloorNoiseLevel    CodecCount16Register2
#define CodecBufferPtr          CodecPtrRegister1

static volatile struct {
    volatile bool ReaderDemodFinished;
    volatile bool CardDemodFinished;
} Flags = { 0 };

typedef enum {
    DEMOD_VCD_SOC_STATE,
    DEMOD_VCD_1_OUT_OF_4_STATE,
    DEMOD_VCD_1_OUT_OF_256_STATE,
    DEMOD_VICC_SOC_STATE,
    DEMOD_VICC_DATA,
} DemodSniffStateType;

static volatile uint8_t ShiftRegister;
static volatile uint8_t ByteCount;
static volatile uint8_t ReaderByteCount;
static volatile uint8_t CardByteCount;
static volatile uint16_t SampleDataCount;
static volatile uint8_t bBrokenFrame;
static bool bAutoThreshold = true;

/////////////////////////////////////////////////
// VCD->VICC
// (Code adapted from ISO15693 codec)
/////////////////////////////////////////////////

/* This functions resets all global variables used in the codec and enables interrupts to wait for reader data */
void ReaderSniffInit(void) {
    /* Reset global variables to default values */
    CodecBufferPtr = CodecBuffer;
    Flags.ReaderDemodFinished = 0;
    Flags.CardDemodFinished = 0;
    StateRegister = DEMOD_VCD_SOC_STATE;
    DataRegister = 0;
    SampleRegister = 0;
    BitSampleCount = 0;
    SampleDataCount = 0;
    ModulationPauseCount = 0;
    ByteCount = 0;
    ReaderByteCount = 0; /* Clear direction-specific counters */
    CardByteCount = 0;
    ShiftRegister = 0;
    bBrokenFrame = 0;

    /* Activate Power for demodulator */
    CodecSetDemodPower(true);

    /* Configure sampling-timer free running and sync to first modulation-pause */
    /* Set clock source for TCD0 to ISO15693_SAMPLE_CLK = TC_CLKSEL_DIV2_gc = System Clock / 2.
     * Since the Chameleon is clocked at 13.56*2 MHz (see Makefile), this counter will hit at the same frequency of reader field.
     * From 14.12.1 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_SAMPLING.CTRLA = ISO15693_READER_SAMPLE_CLK;
    /* Set up TCD0 action/source:
     *  - Event Action: Restart timer (reset PER register)
     *  - Event Source Select: trigger on CODEC_TIMER_MODSTART_EVSEL = TC_EVSEL_CH0_gc = Event Channel 0
     * From 14.12.4 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    /* Resets the sampling counter (TCD0) to 0 - From 14.12.12 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CNT = 0;
    /* Set the period for TCD0 to ISO15693_SAMPLE_PERIOD - 1 - From 14.12.14 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.PER = ISO15693_READER_SAMPLE_PERIOD - 1;
    /* Set Compare Channel C (CCC) period to half bit period - 1. (- 14 to compensate ISR timing overhead) - From 14.12.16 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CCC = ISO15693_READER_SAMPLE_PERIOD / 2 - 14 - 1;
    /**
     * Temporarily disable Compare Channel C (CCC) interrupts. They'll be enabled later
     * in isr_SNIFF_ISO15693_CODEC_DEMOD_READER_IN_INT0_VECT once we sensed the reader
     * is sending data and we're in sync with the pulses.
     * From 14.12.7 [8331F–AVR–04/2013]
     */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    /* Clear Compare Channel C (CCC) interrupt Flags - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;


    /* Start looking out for modulation pause via interrupt. */
    /* Clear PORTB interrupt flags - From 13.13.13 [8331F–AVR–04/2013] */
    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT0IF_bm;
    /* Use PIN1 as a source for modulation pauses. Will trigger PORTB Interrput 0 - From 13.13.11 [8331F–AVR–04/2013] */
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
}


/* This function implements CODEC_DEMOD_IN_INT0_VECT interrupt vector.
 * It is called when a pulse is detected in CODEC_DEMOD_IN_PORT (PORTB).
 * The relevatn interrupt vector is registered to CODEC_DEMOD_IN_MASK0 (PIN1) via:
 * CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
 * and unregistered writing the INT0MASK to 0
 */
ISR_SHARED isr_SNIFF_ISO15693_CODEC_DEMOD_READER_IN_INT0_VECT(void) {
    /* Start sample timer CODEC_TIMER_SAMPLING (TCD0).
     * Set Counter Channel C (CCC) with relevant bitmask (TC0_CCCIF_bm),
     * the period for clock sampling is specified in ReaderSniffInit.
     */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;
    /* Sets register INTCTRLB to TC_CCCINTLVL_HI_gc = (0x03<<4) to enable compare/capture for high level interrupts on Channel C (CCC) */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_HI_gc;

    /* Disable this interrupt as we've already sensed the relevant pulse and will use our internal clock from now on */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
}

/* This function is registered to CODEC_TIMER_SAMPLING (TCD0)'s Counter Channel C (CCC).
 * When the timer is enabled, this is called on counter's overflow
 *
 * It demodulates bits received from the reader and saves them in CodecBuffer.
 *
 * It disables its own interrupt when receives an EOF (calling ISO15693_EOC) or when it receives garbage
 */
ISR_SHARED SNIFF_ISO15693_READER_CODEC_TIMER_SAMPLING_CCC_VECT(void) {
    /* Shift demod data */
    SampleRegister = (SampleRegister << 1) | (!(CODEC_DEMOD_IN_PORT.IN & CODEC_DEMOD_IN_MASK) ? 0x01 : 0x00);

    if (++BitSampleCount == 8) {
        BitSampleCount = 0;
        switch (StateRegister) {
            case DEMOD_VCD_SOC_STATE:
                if (SampleRegister == VCD_SOC_1_OF_4_CODE) {
                    StateRegister = DEMOD_VCD_1_OUT_OF_4_STATE;
                    SampleDataCount = 0;
                    ModulationPauseCount = 0;
                } else if (SampleRegister == VCD_SOC_1_OF_256_CODE) {
                    StateRegister = DEMOD_VCD_1_OUT_OF_256_STATE;
                    SampleDataCount = 0;
                } else { // No SOC. Restart and try again, we probably received garbage.
                    Flags.ReaderDemodFinished = 1;
                    Flags.CardDemodFinished = 1; /* Mark Card Demod as finished as well to restart clean in CodecTagk (no bytes received -> no log) */
                    /* Sets timer off for CODEC_TIMER_SAMPLING (TCD0) disabling clock source */
                    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
                    /* Sets register INTCTRLB to 0 to disable all compare/capture interrupts */
                    CODEC_TIMER_SAMPLING.INTCTRLB = 0;
                }
                break;

            case DEMOD_VCD_1_OUT_OF_4_STATE:
                if (SampleRegister == VCD_EOC_CODE) {
                    SNIFF_ISO15693_READER_EOC_VCD();
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

            case DEMOD_VCD_1_OUT_OF_256_STATE:
                if (SampleRegister == VCD_EOC_CODE) {
                    SNIFF_ISO15693_READER_EOC_VCD();
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
        SampleRegister = 0;
    }
    SampleDataCount++;
}

/* This function is called from isr_ISO15693_CODEC_TIMER_SAMPLING_CCC_VECT
 * when we have 8 bits in SampleRegister and they represent an end of frame.
 */
INLINE void SNIFF_ISO15693_READER_EOC_VCD(void) {
    /* Mark reader data as received */
    Flags.ReaderDemodFinished = 1;
    ReaderByteCount = ByteCount; /* Copy to direction-specific variable */

    /* Sets timer off for TCD0, disabling clock source. We're done receiving data from reader and don't need to probe the antenna anymore - From 14.12.1 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    /* Disable event action for CODEC_TIMER_SAMPLING (TCD0) - From 14.12.4 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    /* Disable compare/capture interrupts on Channel C - From 14.12.7 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    /* Clear Compare Channel C (CCC) interrupt Flags - From 14.12.10 [8331F–AVR–04/2013] */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;

    /* And initialize VICC->VCD sniffer */
    CardSniffInit();
}

/////////////////////////////////////////////////
// VICC->VCD
/////////////////////////////////////////////////


/* Inline function to set the threshold, if bAutoThreshold is enabled*/
INLINE void SetDacCh0Data(uint16_t ch0data) {
    /* TODO: Check if either of the branches can be avoided */
    /* to optimize the performance */
    if (bAutoThreshold) {
        if (ch0data < 0xFFF)
            DACB.CH0DATA = ch0data;
        else
            DACB.CH0DATA = 0xFFF;
    }
    return;
}

/* Initialize card sniffing options
Note: Currently implemented only single subcarrier SOC detection
 */
INLINE void CardSniffInit(void) {
    /* Reinit state variables */
    CodecBufferPtr = CodecBuffer2;
    ByteCount = 0;

    if (CodecBuffer[0] & ISO15693_REQ_SUBCARRIER_DUAL) {
        /**
         * No support for dual subcarrier
         */
        LogEntry(LOG_INFO_GENERIC, "NO DUAL SC", 10);

        CardByteCount = ByteCount; /* Copy to direction-specific variable */

        Flags.CardDemodFinished = 1;
        return;
    }

    /**
     * Prepare ADC for antenna field sampling
     * Use conversion channel 1 (0 is already used for reader field RSSI sensing), attached to
     * CPU pin 42 (DEMOD-READER/2.3C).
     * Use conversion channel 2, attached to CPU pin 3 (DEMOD/2.3C).
     *
     * Values from channel 1 (DEMOD-READER/2.3C) will be used to determine pulses levels. This channel is used
     * because the signal shape from DEMOD-READER is easier to analyze with our limited resources and will
     * more likely yield useful values.
     * Values from channel 2 (DEMOD/2.3C) will be used to identify the first pulse threshold: ADC channel 2
     * is sampling on the same channel where analog comparator is comparing values, thus the value from
     * channel 2 is the only useful threshold to identify the first pulse.
     */
    ADCA.PRESCALER = ADC_PRESCALER_DIV4_gc; /* Increase ADC clock speed from default setting in AntennaLevel.h */
    ADCA.CTRLB |= ADC_FREERUN_bm; /* Set ADC as free running */
    ADCA.EVCTRL = ADC_SWEEP_012_gc; /* Enable free running sweep on channel 0, 1 and 2 */
    ADCA.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN2_gc; /* Sample PORTA Pin 2 (DEMOD-READER/2.3C) in channel 1 (same pin the analog comparator is comparing to) */
    ADCA.CH1.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc; /* Single-ended input, no gain */
    ADCA.CH2.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc; /* Sample PORTA Pin 7 (DEMOD/2.3C) in channel 2 */
    ADCA.CH2.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc; /* Single-ended input, no gain */

    /**
     * CODEC_READER_TIMER will now be used to timeout VICC sniffing if no data is received before period overflow.
     * The overflow interrupt has to be manually deactivated once meaningful data is received, no events will clear it.
     *
     * PER = maximum card wait time (1500 us).
     */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV256_gc; /* Clocked at 1/256 CPU clock */
    CODEC_TIMER_SAMPLING.PER = 160; /* ~1500 us card response timeout */
    CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_HI_gc; /* Enable overflow (SOF timeout) interrupt */
    CODEC_TIMER_SAMPLING.CTRLFSET = TC_CMD_RESTART_gc; /* Reset timer once it has been configured */

    /**
     * CODEC_TIMER_TIMESTAMPS (TCD1) will be used to count the peaks identified by ACA while sniffing VICC data
     *
     * CCA = 3 pulses, to update the threshold to a more suitable value for upcoming pulses
     */
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_EVCH2_gc; /* Using Event channel 2 as an input */
    CODEC_TIMER_TIMESTAMPS.CCA = 3;
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCAINTLVL_HI_gc; /* Enable CCA interrupt */
    CODEC_TIMER_TIMESTAMPS.CTRLFSET = TC_CMD_RESTART_gc; /* Reset counter once it has been configured */

    /**
     * CODEC_TIMER_LOADMOD (TCE0) has multiple usages:
     *  - Period overflow samples second half-bit (called 37,76 us after bit start) - while decoding data
     *  - Compare channel A:
     *      - filters erroneous VICC->VCD SOC identification
     *      - prepares the codec for data demodulation when a SOF is found
     *  - Compare channel C samples the first half-bit (called 18,88 us after bit start) - while decoding data
     *
     * It is restarted on every event on event channel 2 until the first 24 pulses of the SOC have been received, then
     * it restarts on period overflow (after one full logic bit).
     *
     * Interrupt on compare channel A will be called every 64 clock cycles, unless this timer is reset by a new
     * pulse. This means that interrupt on CCA will be called after some pulses, be it one or more.
     * When CCA is called and we have received too few pulses, it was just noise on the field, so we have to start
     * searching again for a SOF. On the other hand, if we received multiple pulses, we can finally start demodulation.
     * Also, compare channel A can then be disabled.
     *
     * Interrupt on compare channel C will be called after the first half-bit and will save it in the samples register.
     *
     * Overflow interrupt will be called after the second half-bit and will save it in the samples register.
     * Every 16 half-bits, this will also convert the samples to a byte and store it in the codec buffer.
     * It will also check for EOF to stop card decoding.
     *
     * PER = one logic bit duration (27,76 us)
     * CCA = duration of a single pulse (2,36 us: half hi + half low)
     * CCC = half-bit duration (18,88 us)
     */
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_DIV1_gc; /* Clocked at 27.12 MHz */
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH2_gc; /* Restart this timer on every event on channel 2 (pulse detected by AC) */
    CODEC_TIMER_LOADMOD.PER = 1024 - 8; /* One full logic bit duration, slightly shorter on first run to make room for SOC detection algoritm */
    CODEC_TIMER_LOADMOD.PERBUF = 1024 - 1; /* One full logic bit duration - 1 (accommodate for small clock drift) */
    CODEC_TIMER_LOADMOD.CCA = 64; /* Single pulse width */
    CODEC_TIMER_LOADMOD.CCC = 512 - 8; /* Same as PER, use a slightly shorter CCC period on first run */
    CODEC_TIMER_LOADMOD.CCCBUF = 512; /* After first CCC, use actual CCC period (will be written when UPDATE condition is met, thus after first period hit) */
    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc; /* Keep overflow interrupt (second half-bit) disabled */
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCAINTLVL_OFF_gc | TC_CCBINTLVL_OFF_gc | TC_CCCINTLVL_OFF_gc; /* Keep all compare interrupt disabled, they will be enabled later on */
    CODEC_TIMER_LOADMOD.CTRLFSET = TC_CMD_RESTART_gc; /* Reset timer */

    /**
     * Get current signal amplitude and record it as "silence"
     *
     * This can't be moved closer to ADC configuration since the ADC has to be populated with valid
     * values and it takes 7*4 clock cycles to do so (7 ADC stages * 4 ADC clock prescaler)
     */
    uint32_t accumulator;
    accumulator = ADCA.CH2RES - ANTENNA_LEVEL_OFFSET; /* PORTA Pin 7 (DEMOD/2.3C) - CPU pin 7 */
    for (uint8_t i = 0; i < 127; i++) { /* Add 127 more values */
        accumulator += ADCA.CH2RES - ANTENNA_LEVEL_OFFSET;
    }
    DemodFloorNoiseLevel = (uint16_t)(accumulator >> 7); /* Divide by 2^7=128 */
    /**
     * Typical values with my CR95HF reader:
     * ReaderFloorNoiseLevel: ~900 (measured on ADCA.CH1RES)
     * DemodFloorNoiseLevel: ~1500
    */

    /* Reconfigure ADC to sample only the first 2 channels from now on (no need for CH2 once the noise level has been found) */
    ADCA.EVCTRL = ADC_SWEEP_01_gc;

    /* Write threshold to the DAC channel 0 (connected to Analog Comparator positive input) */
    SetDacCh0Data(DemodFloorNoiseLevel + (DemodFloorNoiseLevel >> 3)); /* Slightly increase DAC output to ease triggering */


    /**
     * Finally, now that we have the DAC set up, configure analog comparator A (the only one in this MCU)
     * channel 0 (the only one that can be connecte to the DAC) to recognize carrier pulses modulated by
     * the VICC against the correct threshold coming from the DAC.
     */
    ACA.AC0MUXCTRL = AC_MUXPOS_DAC_gc | AC_MUXNEG_PIN7_gc; /* Tigger when DAC signal is above PORTA Pin 7 (DEMOD/2.3C) */
    /* enable AC | high speed mode | large hysteresis | sample on rising edge | high level interrupts */
    /* Hysteresis is not actually needed, but appeared to be working and sounds like it might be more robust */
    ACA.AC0CTRL = AC_ENABLE_bm | AC_HSMODE_bm | AC_HYSMODE_LARGE_gc | AC_INTMODE_RISING_gc | AC_INTLVL_HI_gc;

    /* This function ends ~100 us after last VCD pulse */
}

/**
 * This function restores all the configured peripherals (ADC/AC/timers) to a clean state
 *
 * Not inlined, since once we need to call this, there won't be any strict timing constraint
 */
void CardSniffDeinit(void) {
    /* Restore ADC settings as per AntennaLevel.h */
    ADCA.PRESCALER = ADC_PRESCALER_DIV32_gc; /* Reduce clock */
    ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc; /* Stop free running ADC */
    ADCA.EVCTRL = 0; /* Stop channel sweep or any ADC event */
    /* Ignore channel 1 and 2 mux settings (no "off" state) */

    /* Disable all timers interrupts */
    CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_OFF_gc;
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCAINTLVL_OFF_gc;
    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCAINTLVL_OFF_gc | TC_CCBINTLVL_OFF_gc | TC_CCCINTLVL_OFF_gc;

    /* Stop timer/counters disabling clock sources */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;

    /* Reset ACA AC0 to default setting */
    ACA.AC0MUXCTRL = AC_MUXPOS_DAC_gc | AC_MUXNEG_PIN7_gc; /* This actually was unchanged */
    ACA.AC0CTRL = CODEC_AC_DEMOD_SETTINGS; /* Disable interrupt as well */
}


/**
 * This interrupt is called on every rising edge sensed by the analog comparator
 * and it enables the spurious pulse filtering interrupt (CODEC_TIMER_LOADMOD CCA).
 * If we did not receive a SOC but, indeed, noise, this interrupt will be enabled again.
 */
ISR_SHARED isr_SNIFF_ISO15693_ACA_AC0_VECT(void) {
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCAINTLVL_HI_gc; /* Enable level 0 CCA interrupt to filter spurious pulses and find SOC */

    ACA.AC0CTRL = AC_ENABLE_bm | AC_HSMODE_bm | AC_HYSMODE_LARGE_gc | AC_INTMODE_RISING_gc | AC_INTLVL_OFF_gc; /* Disable this interrupt */

    SetDacCh0Data(DemodFloorNoiseLevel + (DemodFloorNoiseLevel >> 2)); /* Blindly increase threshold after 1 pulse */

    /* Note: by the time the DAC has changed its output, we're already after the 2nd pulse */
}

/**
 * This interrupt is called after 3 subcarrier pulses and increases the threshold
 */
ISR_SHARED isr_SNIFF_ISO15693_CODEC_TIMER_TIMESTAMPS_CCA_VECT(void) {

    uint16_t TempRead = ADCA.CH1RES; /* Can't possibly be greater than 0xFFF since the dac has 12 bit res */

    SetDacCh0Data(TempRead - ANTENNA_LEVEL_OFFSET); /* Further increase DAC output after 3 pulses with value from PORTA Pin 2 (DEMOD-READER/2.3) */
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCAINTLVL_OFF_gc; /* Disable this interrupt */
}

/**
 * This interrupt is called every 64 clock cycles (= once per pulse) unless the timer is reset.
 * This means it will be invoked only if, after a pulse, we don't receive another one.
 *
 * This will either find a noise or the end of the SOC. If we've received noise, most likely,
 * we received few pulses. The soc is made of 24 pulses, so if VICC modulation actually started,
 * we should have a relevant number of pulses
 */
ISR_SHARED isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_CCA_VECT(void) {
    if (CODEC_TIMER_TIMESTAMPS.CNT < 15) {
        /* We most likely received garbage */

        CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCAINTLVL_OFF_gc; /* Disable all compare interrupts, including this one */

        SetDacCh0Data(DemodFloorNoiseLevel + (DemodFloorNoiseLevel >> 3)); /* Restore DAC output (AC negative comparation threshold) to pre-AC0 interrupt update value */

        ACA.AC0CTRL |= AC_INTLVL_HI_gc; /* Re-enable analog comparator interrupt to search for another pulse */

        CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCAINTLVL_HI_gc; /* Re enable CCA interrupt in case it was triggered and then is now disabled */
    } else {
        /**
         * Received a lot of pulses, we can assume it was the SOF, prepare interrupts for data demodulation.
         * Data demodulation interrupt will take care of checking if we receive the two remaining half-bits
         * of the sof and will then convalidate that these pulses were actually part of the SOF
         */

        CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCCINTLVL_HI_gc; /* Enable CCC (first half-bit decoding), disable CCA (spurious pulse timeout) */
        CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_HI_gc; /* Enable overflow (second half-bit decoding) interrupt */
        CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc; /* Disable timer resetting on every AC0 event */

        /**
         * Prepare registers for data demodulation
         * This is a bit like cheating. We assumed this is a real SOC, so we can pretend we've already received
         * the first 3 unmodulated and 3 modulated periods (binary: 0b000111) which compose the SOC.
         * The two upcoming half-bits (0b01) will be shifted in during data decoding.
         */
        SampleRegister = 0b000111; /* = 0x7 */
        BitSampleCount = 4 + 3; /* Pretend we've already received 8 empty half-bits and the above 6 half-bits */
        StateRegister = DEMOD_VICC_SOC_STATE;
    }

    CODEC_TIMER_TIMESTAMPS.CNT = 0; /* Clear pulses counter nonetheless */
}

/**
 * This interrupt is called on the first half-bit
 */
ISR(CODEC_TIMER_LOADMOD_CCC_VECT) {
    /**
     * This interrupt is called on every odd half-bit, thus we don't need to do any check,
     * just append to the sample register.
     */
    SampleRegister = (SampleRegister << 1) | (CODEC_TIMER_TIMESTAMPS.CNT > 4); /* Using 4 as a discriminating factor to allow for slight errors in pulses counting. */
    /* Don't increase BitSampleCount, since this is only the first half of a bit */

    CODEC_TIMER_TIMESTAMPS.CNT = 0; /* Clear count register for next half-bit */
}

/**
 * This interrupt handles the VICC SOF timeout when the card does not answer
 * and restarts reader sniffing
 */
ISR(CODEC_TIMER_SAMPLING_OVF_VECT) {
    Flags.CardDemodFinished = 1;

    /* Call cleanup function */
    CardSniffDeinit();
}

/**
 * This interrupt is called on the second bit-half to decode it.
 * It replaces isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_OVF_VECT_timeout once the SOF is received
 */
ISR_SHARED isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_OVF_VECT(void) {
    /**
     * This interrupt is called on every even half-bit, we then need to check the content of the register
     */
    SampleRegister = (SampleRegister << 1) | (CODEC_TIMER_TIMESTAMPS.CNT > 4); /* Using 4 as a discriminating factor to allow for slight errors in pulses counting. */
    BitSampleCount++;
    CODEC_TIMER_TIMESTAMPS.CNT = 0; /* Clear count register for next half-bit */

    // char tmpBuf[10];
    // snprintf(tmpBuf, 10, "BSC: %d\n", BitSampleCount);
    // TerminalSendString(tmpBuf);

    if (BitSampleCount == 8) { /* We have 16 half-bits in SampleRegister at this point */
        BitSampleCount = 0;

        switch (StateRegister) {
            case DEMOD_VICC_SOC_STATE:
                if (SampleRegister == VICC_SOC_CODE) {
                    /* We've actually received a SOF after the previous (mostly unknown) train of pulses */
                    CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_OFF_gc; /* Disable VICC SOF timeout handler */
                    StateRegister = DEMOD_VICC_DATA;
                } else {
                    /* No SOC. The train of pulses was actually garbage. */

                    /* Restore DAC output to intial value */
                    SetDacCh0Data(DemodFloorNoiseLevel + (DemodFloorNoiseLevel >> 3));


                    /* Re enable AC interrupt */
                    ACA.AC0CTRL = AC_ENABLE_bm | AC_HSMODE_bm | AC_HYSMODE_LARGE_gc | AC_INTMODE_RISING_gc | AC_INTLVL_HI_gc;

                    /* Restore other interrupts as per original setup (same as CardSniffInit) */
                    CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCAINTLVL_HI_gc; /* Re enable 3rd pulse thresh update */
                    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc; /* Disable overflow interrupt (second half-bit) */
                    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCAINTLVL_OFF_gc | TC_CCBINTLVL_OFF_gc | TC_CCCINTLVL_OFF_gc; /* Disable all compare channel interrupts */

                    /* Reset timers */
                    CODEC_TIMER_TIMESTAMPS.CTRLFSET = TC_CMD_RESTART_gc; /* Restart peak counter */
                    CODEC_TIMER_LOADMOD.CTRLFSET = TC_CMD_RESTART_gc; /* Reset pulse train timer */

                    /* Reset periods which were possibly modified */
                    CODEC_TIMER_LOADMOD.PER = 1024 - 8; /* One full logic bit duration, slightly shorter on first run to make room for SOC detection algoritm */
                    CODEC_TIMER_LOADMOD.PERBUF = 1024 - 1; /* One full logic bit duration - 1 (accommodate for small clock drift) */
                    CODEC_TIMER_LOADMOD.CCC = 512 - 8; /* Same as PER, use a slightly shorter CCC period on first run */
                    CODEC_TIMER_LOADMOD.CCCBUF = 512; /* After first CCC, use actual CCC period (will be written when UPDATE condition is met, thus after first period hit) */

                    /* No need to (dis|en)able CODEC_TIMER_SAMPLING OVF interrupt (we're still waiting for a valid SOF and that interrupt wasn't disabled) */
                }
                break;

            case DEMOD_VICC_DATA:
                if (SampleRegisterL == VICC_EOC_CODE) {
                    Flags.CardDemodFinished = 1;

                    CardByteCount = ByteCount; /* Copy to direction-specific variable */

                    /* Call cleanup function */
                    CardSniffDeinit();
                } else if (
                    (SampleRegisterL & 0b00000011) == 0b00000011 || /* Ugly, I know */
                    (SampleRegisterL & 0b00000011) == 0b00000000 ||
                    (SampleRegisterL & 0b00001100) == 0b00001100 ||
                    (SampleRegisterL & 0b00001100) == 0b00000000 ||
                    (SampleRegisterL & 0b00110000) == 0b00110000 ||
                    (SampleRegisterL & 0b00110000) == 0b00000000 ||
                    (SampleRegisterL & 0b11000000) == 0b11000000 ||
                    (SampleRegisterL & 0b11000000) == 0b00000000 ||
                    (SampleRegisterH & 0b00000011) == 0b00000011 ||
                    (SampleRegisterH & 0b00000011) == 0b00000000 ||
                    (SampleRegisterH & 0b00001100) == 0b00001100 ||
                    (SampleRegisterH & 0b00001100) == 0b00000000 ||
                    (SampleRegisterH & 0b00110000) == 0b00110000 ||
                    (SampleRegisterH & 0b00110000) == 0b00000000 ||
                    (SampleRegisterH & 0b11000000) == 0b11000000 ||
                    (SampleRegisterH & 0b11000000) == 0b00000000
                ) {
                    /**
                     * Check for invalid data coding errors (11 or 00 bit-halfs).
                     * Mark broken frame and stop here: we have no knowledge about where the EOF could be
                     */

                    Flags.CardDemodFinished = 1;
                    bBrokenFrame = 1;
                    CardByteCount = ByteCount; /* Copy to direction-specific variable */

                    /* Call cleanup function */
                    CardSniffDeinit();

                } else {
                    DataRegister  = ((SampleRegisterL & 0b00000011) == 0b00000001) << 3;
                    DataRegister |= ((SampleRegisterL & 0b00001100) == 0b00000100) << 2;
                    DataRegister |= ((SampleRegisterL & 0b00110000) == 0b00010000) << 1;
                    DataRegister |= ((SampleRegisterL & 0b11000000) == 0b01000000);  /* Bottom 2 bits */
                    DataRegister |= ((SampleRegisterH & 0b00000011) == 0b00000001) << 7;
                    DataRegister |= ((SampleRegisterH & 0b00001100) == 0b00000100) << 6;
                    DataRegister |= ((SampleRegisterH & 0b00110000) == 0b00010000) << 5;
                    DataRegister |= ((SampleRegisterH & 0b11000000) == 0b01000000) << 4;

                    *CodecBufferPtr = DataRegister;
                    CodecBufferPtr++;
                    ByteCount++;
                    if (ByteCount == 256) {
                        /* Somehow missed the EOF and running out of buffer */
                        Flags.CardDemodFinished = 1;

                        /* Call cleanup function */
                        CardSniffDeinit();
                    };
                }
                break;
        }
    }

}


/////////////////////////////////////////////////
// External interface
/////////////////////////////////////////////////

void SniffISO15693CodecInit(void) {
    CodecInitCommon();

    /**
     * Register function handlers to shared ISR
     */
    /* CODEC_TIMER_SAMPLING (TCD0)'s Counter Channel C (CCC) - Reader demod */
    isr_func_TCD0_CCC_vect = &SNIFF_ISO15693_READER_CODEC_TIMER_SAMPLING_CCC_VECT;
    /* CODEC_DEMOD_IN_PORT (PORTB) interrupt 0 - Reader demod */
    isr_func_CODEC_DEMOD_IN_INT0_VECT = &isr_SNIFF_ISO15693_CODEC_DEMOD_READER_IN_INT0_VECT;
    /* CODEC_TIMER_TIMESTAMPS (TCD1)'s Counter Channel C (CCC) - Card demod */
    isr_func_CODEC_TIMER_TIMESTAMPS_CCA_VECT = &isr_SNIFF_ISO15693_CODEC_TIMER_TIMESTAMPS_CCA_VECT; /* Updated threshold after 3 pulses */
    /* CODEC_TIMER_LOADMOD (TCE0)'s Counter Channel A (CCA) and overflow (OVF) - Card demod */
    isr_func_CODEC_TIMER_LOADMOD_CCA_VECT = &isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_CCA_VECT; /* Spurious pulses and SOC detection */
    /* CCC (first half-bit decoder) ISR is not shared, thus doesn't need to be assigned here */
    isr_func_CODEC_TIMER_LOADMOD_OVF_VECT = &isr_SNIFF_ISO15693_CODEC_TIMER_LOADMOD_OVF_VECT; /* Second half-bit decoder */
    /* Analog comparator A (ACA) channel 0 interrupt handler - Card demod */
    isr_func_ACA_AC0_vect = &isr_SNIFF_ISO15693_ACA_AC0_VECT; /* Detect first card pulse and enable demod interrupts */

    /**
     * Route events from the analog comparator (which will be configured later) on the event system
     * using channel 2 (channel 0 and 1 are used by the antenna).
     * These events will be counted by CODEC_TIMER_TIMESTAMPS and will (sometimes)
     * reset CODEC_TIMER_LOADMOD.
     */
    EVSYS.CH2MUX = EVSYS_CHMUX_ACA_CH0_gc; /* Route analog comparator channel 0 events on event system channel 2 */

    /* Change DAC reference source to Internal 1V (same as ADC source) */
    DACB.CTRLC = DAC_REFSEL_INT1V_gc;

    ReaderSniffInit();
}

/************************************************************
    Function used by the Terminal command to display (GET)
    the state of the Autothreshold Feature.
************************************************************/
bool SniffISO15693GetAutoThreshold(void) {
    return bAutoThreshold;
}

/************************************************************
    Function used by the Application level to disable the
    Autothreshold Feature.
    If it is disabled: The threshold will be taken from
    GlobalSettings.ActiveSettingPtr->ReaderThreshold
************************************************************/
void SniffISO15693CtrlAutoThreshold(bool enable) {
    bAutoThreshold = enable;
    return;
}

/************************************************************
    Function used by the Application level to get the FloorNoise
    FloorNoise can be used to define the scanning range for the
    Autocalibration.
************************************************************/
uint16_t SniffISO15693GetFloorNoise(void) {
    return DemodFloorNoiseLevel;
}

void SniffISO15693CodecDeInit(void) {
    /* Gracefully shutdown codec */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;

    /* Reset global variables to default values */
    CodecBufferPtr = CodecBuffer;
    Flags.ReaderDemodFinished = 0;
    Flags.CardDemodFinished = 0;
    StateRegister = DEMOD_VCD_SOC_STATE;
    DataRegister = 0;
    SampleRegister = 0;
    BitSampleCount = 0;
    SampleDataCount = 0;
    ModulationPauseCount = 0;
    ByteCount = 0;
    ShiftRegister = 0;

    /* Disable sample timer */
    /* Sets timer off for CODEC_TIMER_SAMPLING (TCD0) disabling clock source */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    /* Disable event action for CODEC_TIMER_SAMPLING (TCD0) */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    /* Sets register INTCTRLB to TC_CCCINTLVL_OFF_gc = (0x00<<4) to disable compare/capture C interrupts */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;
    /* Restore Counter Channel C (CCC) interrupt mask (TC0_CCCIF_bm) */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;

    CodecSetDemodPower(false);

    /* Stop AC0 events routing on channel 2 */
    EVSYS.CH2MUX = EVSYS_CHMUX_OFF_gc;

    /* Restore default DAC reference to Codec.h setting */
    DACB.CTRLC = DAC_REFSEL_AVCC_gc;
}

void SniffISO15693CodecTask(void) {
    if (Flags.ReaderDemodFinished) {
        Flags.ReaderDemodFinished = 0;

        if (ReaderByteCount > 0) {
            LogEntry(LOG_INFO_CODEC_SNI_READER_DATA, CodecBuffer, ReaderByteCount);
            LEDHook(LED_CODEC_RX, LED_PULSE);

            SniffTrafficSource = TRAFFIC_READER;
            ApplicationProcess(CodecBuffer, ReaderByteCount);
        }

    }

    if (Flags.CardDemodFinished) {
        Flags.CardDemodFinished = 0;

        if (CardByteCount > 0) {
            LogEntry(LOG_INFO_CODEC_SNI_CARD_DATA, CodecBuffer2, CardByteCount);
            LEDHook(LED_CODEC_RX, LED_PULSE);

            SniffTrafficSource = TRAFFIC_CARD;
            ApplicationProcess(CodecBuffer2, CardByteCount);
            /* Note: the application might want to know if the frame is broken, need to extern bBrokenFrame */
        }

        if (bBrokenFrame) {
            bBrokenFrame = 0;
            LogEntry(LOG_INFO_GENERIC, "BROKEN CARD FRAME", 17);
        }

        ReaderSniffInit();
    }
}
