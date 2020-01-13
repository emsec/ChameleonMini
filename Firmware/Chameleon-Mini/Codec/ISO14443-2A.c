/*
 * ISO14443A.c
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#include "ISO14443-2A.h"
#include "../System.h"
#include "../Application/Application.h"
#include "../LEDHook.h"
#include "Codec.h"
#include "Log.h"

/* Sampling is done using internal clock, synchronized to the field modulation.
 * For that we need to convert the bit rate for the internal clock. */
#define SAMPLE_RATE_SYSTEM_CYCLES		((uint16_t) (((uint64_t) F_CPU * ISO14443A_BIT_RATE_CYCLES) / CODEC_CARRIER_FREQ) )

#define ISO14443A_MIN_BITS_PER_FRAME		7

static volatile struct {
    volatile bool DemodFinished;
    volatile bool LoadmodFinished;
} Flags = { 0 };

typedef enum {
    /* Demod */
    DEMOD_DATA_BIT,
    DEMOD_PARITY_BIT,

    /* Loadmod */
    LOADMOD_FDT,
    LOADMOD_START,
    LOADMOD_START_BIT0,
    LOADMOD_START_BIT1,
    LOADMOD_DATA0,
    LOADMOD_DATA1,
    LOADMOD_PARITY0,
    LOADMOD_PARITY1,
    LOADMOD_STOP_BIT0,
    LOADMOD_STOP_BIT1,
    LOADMOD_FINISHED
} StateType;

/* Define pseudo variables to use fast register access. This is useful for global vars */
#define DataRegister	Codec8Reg0
#define StateRegister	Codec8Reg1
#define ParityRegister	Codec8Reg2
#define SampleIdxRegister Codec8Reg2
#define SampleRegister	Codec8Reg3
#define BitSent			CodecCount16Register1
#define BitCount		CodecCount16Register2
#define CodecBufferPtr	CodecPtrRegister1
#define ParityBufferPtr	CodecPtrRegister2

static void StartDemod(void) {
    /* Activate Power for demodulator */
    CodecSetDemodPower(true);

    CodecBufferPtr = CodecBuffer;
    ParityBufferPtr = &CodecBuffer[ISO14443A_BUFFER_PARITY_OFFSET];
    DataRegister = 0;
    SampleRegister = 0;
    SampleIdxRegister = 0;
    BitCount = 0;
    StateRegister = DEMOD_DATA_BIT;

    /* Configure sampling-timer free running and sync to first modulation-pause. */
    CODEC_TIMER_SAMPLING.CNT = 0;                               // Reset the timer count
    CODEC_TIMER_SAMPLING.PER = SAMPLE_RATE_SYSTEM_CYCLES - 1;   // Set Period regisiter
    CODEC_TIMER_SAMPLING.CCA = 0xFFFF; /* CCA Interrupt is not active! */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV1_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCAIF_bm;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCAINTLVL_HI_gc;

    /* Start looking out for modulation pause via interrupt. */
    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT0IF_bm;
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
}

// Find first pause and start sampling
ISR_SHARED isr_ISO14443_2A_TCD0_CCC_vect(void) {
    /* This is the first edge of the first modulation-pause after StartDemod.
     * Now we have time to start
     * demodulating beginning from one bit-width after this edge. */

    /* Sampling timer has been preset to sample-rate and has automatically synced
     * to THIS first modulation pause. Thus after exactly one bit-width from here,
     * an OVF is generated. We want to start sampling with the next bit and use the
     * XYZBUF mechanism of the xmega to automatically double the sampling rate on the
     * next overflow. For this we have to temporarily deactivate the automatical alignment
     * in order to catch next overflow event for updating the BUF registers.
     * We want to sample the demodulated data stream in the first quarter of the half-bit
     * where the pulsed miller encoded is located. */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_SAMPLING.PERBUF = SAMPLE_RATE_SYSTEM_CYCLES / 2 - 1; /* Half bit width */
    CODEC_TIMER_SAMPLING.CCABUF = SAMPLE_RATE_SYSTEM_CYCLES / 8 - 14 - 1; /* Compensate for DIGFILT and ISR prolog */

    /* Setup Frame Delay Timer and wire to EVSYS. Frame delay time is
     * measured from last change in RF field, therefore we use
     * the event channel 1 (end of modulation pause) as the restart event.
     * The preliminary frame delay time chosen here is irrelevant, because
     * the correct FDT gets set automatically after demodulation. */
    CODEC_TIMER_LOADMOD.CNT = 0;
    CODEC_TIMER_LOADMOD.PER = 0xFFFF;
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODEND_EVSEL;
    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_OVFIF_bm;
    CODEC_TIMER_LOADMOD.CTRLA = CODEC_TIMER_CARRIER_CLKSEL;

    /* Disable this interrupt */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
}

// Sampling with timer and demod
ISR(CODEC_TIMER_SAMPLING_CCA_VECT) {
    /* This interrupt gets called twice for every bit to sample it. */
    uint8_t SamplePin = CODEC_DEMOD_IN_PORT.IN & CODEC_DEMOD_IN_MASK;

    /* Shift sampled bit into sampling register */
    SampleRegister = (SampleRegister << 1) | (!SamplePin ? 0x01 : 0x00);

    if (SampleIdxRegister) {
        SampleIdxRegister = 0;
        /* Analyze the sampling register after 2 samples. */
        if ((SampleRegister & 0x07) == 0x07) {
            /* No carrier modulation for 3 sample points. EOC! */
            CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
            CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCAIF_bm;

            /* By this time, the FDT timer is aligned to the last modulation
             * edge of the reader. So we disable the auto-synchronization and
             * let it count the frame delay time in the background, and generate
             * an interrupt once it has reached the FDT. */
            CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;

            if (SampleRegister & 0x08) {
                CODEC_TIMER_LOADMOD.PER = ISO14443A_FRAME_DELAY_PREV1 - 40; /* compensate for ISR prolog */
            } else {
                CODEC_TIMER_LOADMOD.PER = ISO14443A_FRAME_DELAY_PREV0 - 40; /* compensate for ISR prolog */
            }

            StateRegister = LOADMOD_FDT;

            CODEC_TIMER_LOADMOD.INTFLAGS = TC0_OVFIF_bm;
            CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_HI_gc;

            /* Determine if we did not receive a multiple of 8 bits.
             * If this is the case, right-align the remaining data and
             * store it into the buffer. */
            uint8_t RemainingBits = BitCount % 8;
            if (RemainingBits != 0) {
                uint8_t NewDataRegister = DataRegister;

                while (RemainingBits++ < 8) {
                    /* Pad with zeroes to right-align. */
                    NewDataRegister >>= 1;
                }

                /* TODO: Prevent buffer overflow */
                *CodecBufferPtr = NewDataRegister;
            }

            /* Signal, that we have finished sampling */
            Flags.DemodFinished = 1;
        } else {
            /* Otherwise, we check the two sample bits from the bit before. */
            uint8_t BitSample = SampleRegister & 0xC;
            uint8_t Bit = 0;

            if (BitSample != (0x0 << 2)) {
                /* We have a valid bit. decode and process it. */
                if (BitSample & (0x1 << 2)) {
                    /* 01 sequence or 11 sequence -> This is a zero bit */
                    Bit = 0;
                } else {
                    /* 10 sequence -> This is a one bit */
                    Bit = 1;
                }

                if (StateRegister == DEMOD_DATA_BIT) {
                    /* This is a data bit, so shift it into the data register and
                     * hold a local copy of it. */
                    uint8_t NewDataRegister = DataRegister >> 1;
                    NewDataRegister |= (Bit ? 0x80 : 0x00);
                    DataRegister = NewDataRegister;

                    /* Update bitcount */
                    uint16_t NewBitCount = ++BitCount;
                    if ((NewBitCount & 0x07) == 0) {
                        /* We have reached a byte boundary! Store the data register. */
                        /* TODO: Prevent buffer overflow */
                        *CodecBufferPtr++ = NewDataRegister;

                        /* Store bit for determining FDT at EOC and enable parity
                         * handling on next bit. */
                        StateRegister = DEMOD_PARITY_BIT;
                    }

                } else if (StateRegister == DEMOD_PARITY_BIT) {
                    /* This is a parity bit. Store it */
                    *ParityBufferPtr++ = Bit;
                    StateRegister = DEMOD_DATA_BIT;
                } else {
                    /* Should never Happen (TM) */
                }
            } else {
                /* 00 sequence. -> No valid data yet. This also occurs if we just started
                 * sampling and have sampled less than 2 bits yet. Thus ignore. */
            }
        }
    } else {
        /* On odd sample position just sample. */
        SampleIdxRegister = ~SampleIdxRegister;
    }

    /* Make sure the sampling timer gets automatically aligned to the
     * modulation pauses by using the RESTART event.
     * This can be understood as a "poor mans PLL" and makes sure that we are
     * never too far out the bit-grid while sampling. */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
}

// Enumulate as a card to send card responds
ISR(CODEC_TIMER_LOADMOD_OVF_VECT) {
    /* Bit rate timer. Output a half bit on the output. */

    static void *JumpTable[] = {
        [LOADMOD_FDT] = && LOADMOD_FDT_LABEL,
        [LOADMOD_START] = && LOADMOD_START_LABEL,
        [LOADMOD_START_BIT0] = && LOADMOD_START_BIT0_LABEL,
        [LOADMOD_START_BIT1] = && LOADMOD_START_BIT1_LABEL,
        [LOADMOD_DATA0] = && LOADMOD_DATA0_LABEL,
        [LOADMOD_DATA1] = && LOADMOD_DATA1_LABEL,
        [LOADMOD_PARITY0] = && LOADMOD_PARITY0_LABEL,
        [LOADMOD_PARITY1] = && LOADMOD_PARITY1_LABEL,
        [LOADMOD_STOP_BIT0] = && LOADMOD_STOP_BIT0_LABEL,
        [LOADMOD_STOP_BIT1] = && LOADMOD_STOP_BIT1_LABEL,
        [LOADMOD_FINISHED] = && LOADMOD_FINISHED_LABEL
    };

    if ((StateRegister >= LOADMOD_FDT) && (StateRegister <= LOADMOD_FINISHED)) {
        goto *JumpTable[StateRegister];
    } else {
        return;
    }

LOADMOD_FDT_LABEL:
    /* No data has been produced, but FDT has ended. Switch over to bit-grid aligning. */
    CODEC_TIMER_LOADMOD.PER = ISO14443A_BIT_GRID_CYCLES - 1;
    return;

LOADMOD_START_LABEL:
    /* Application produced data. With this interrupt we are aligned to the bit-grid. */
    /* Fallthrough to first bit */

LOADMOD_START_BIT0_LABEL:
    /* Start subcarrier generation, output startbit and align to bitrate. */
    CodecSetLoadmodState(true);
    CodecStartSubcarrier();

    CODEC_TIMER_LOADMOD.PER = ISO14443A_BIT_RATE_CYCLES / 2 - 1;
    StateRegister = LOADMOD_START_BIT1;
    return;


LOADMOD_START_BIT1_LABEL:
    CodecSetLoadmodState(false);
    StateRegister = LOADMOD_DATA0;
    ParityRegister = ~0;
    BitSent = 0;

    /* Prefetch first byte */
    DataRegister = *CodecBufferPtr;
    return;

LOADMOD_DATA0_LABEL:
    if (DataRegister & 1) {
        CodecSetLoadmodState(true);
        ParityRegister = ~ParityRegister;
    } else {
        CodecSetLoadmodState(false);
    }

    StateRegister = LOADMOD_DATA1;
    return;

LOADMOD_DATA1_LABEL:
    if (DataRegister & 1) {
        CodecSetLoadmodState(false);
    } else {
        CodecSetLoadmodState(true);
    }

    DataRegister = DataRegister >> 1;
    BitSent++;

    if ((BitSent % 8) == 0) {
        /* Byte boundary. Load parity bit and output it later. */
        StateRegister = LOADMOD_PARITY0;
    } else if (BitSent == BitCount) {
        /* End of transmission without byte boundary. Don't send parity. */
        StateRegister = LOADMOD_STOP_BIT0;
    } else {
        /* Next bit is data */
        StateRegister = LOADMOD_DATA0;
    }

    return;

LOADMOD_PARITY0_LABEL:
    if (ParityBufferPtr != NULL) {
        if (*ParityBufferPtr) {
            CodecSetLoadmodState(true);
        } else {
            CodecSetLoadmodState(false);
        }
    } else {
        if (ParityRegister) {
            CodecSetLoadmodState(true);
        } else {
            CodecSetLoadmodState(false);
        }
    }
    StateRegister = LOADMOD_PARITY1;
    return;

LOADMOD_PARITY1_LABEL:
    if (ParityBufferPtr != NULL) {
        if (*ParityBufferPtr) {
            CodecSetLoadmodState(false);
        } else {
            CodecSetLoadmodState(true);
        }

        ParityBufferPtr++;
    } else {
        if (ParityRegister) {
            CodecSetLoadmodState(false);
        } else {
            CodecSetLoadmodState(true);
        }

        ParityRegister = ~0;
    }

    if (BitSent == BitCount) {
        /* No data left */
        StateRegister = LOADMOD_STOP_BIT0;
    } else {
        /* Fetch next data and continue sending bits. */
        DataRegister = *++CodecBufferPtr;
        StateRegister = LOADMOD_DATA0;
    }

    return;

LOADMOD_STOP_BIT0_LABEL:
    CodecSetLoadmodState(false);
    StateRegister = LOADMOD_STOP_BIT1;
    return;

LOADMOD_STOP_BIT1_LABEL:
    CodecSetLoadmodState(false);
    StateRegister = LOADMOD_FINISHED;
    return;

LOADMOD_FINISHED_LABEL:
    /* We have written all of our bits. Deactivate the loadmod
     * timer. Also disable the bit-rate interrupt again. And
     * stop the subcarrier divider. */
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_LOADMOD.INTCTRLA = 0;
    CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OFF, ISO14443A_SUBCARRIER_DIVIDER);

    /* Signal application that we have finished loadmod */
    Flags.LoadmodFinished = 1;
    return;
}

void ISO14443ACodecInit(void) {
    /* Initialize some global vars and start looking out for reader commands */
    Flags.DemodFinished = 0;
    Flags.LoadmodFinished = 0;

    isr_func_TCD0_CCC_vect = &isr_Reader14443_2A_TCD0_CCC_vect;
    isr_func_CODEC_DEMOD_IN_INT0_VECT = &isr_ISO14443_2A_TCD0_CCC_vect;
    CodecInitCommon();
    StartDemod();
}

void ISO14443ACodecDeInit(void) {
    /* Gracefully shutdown codec */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;

    Flags.DemodFinished = 0;
    Flags.LoadmodFinished = 0;

    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCAINTLVL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCAIF_bm;


    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_OVFIF_bm;

    CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OFF, 0);
    CodecSetDemodPower(false);
    CodecSetLoadmodState(false);

}

void ISO14443ACodecTask(void) {
    if (Flags.DemodFinished) {
        Flags.DemodFinished = 0;
        /* Reception finished. Process the received bytes */
        uint16_t DemodBitCount = BitCount;
        uint16_t AnswerBitCount = ISO14443A_APP_NO_RESPONSE;

        if (DemodBitCount >= ISO14443A_MIN_BITS_PER_FRAME) {
            // For logging data
            LogEntry(LOG_INFO_CODEC_RX_DATA, CodecBuffer, (DemodBitCount + 7) / 8);
            LEDHook(LED_CODEC_RX, LED_PULSE);

            /* Call application if we received data */
            AnswerBitCount = ApplicationProcess(CodecBuffer, DemodBitCount);

            if (AnswerBitCount & ISO14443A_APP_CUSTOM_PARITY) {
                /* Application has generated it's own parity bits.
                 * Clear this option bit. */
                AnswerBitCount &= ~ISO14443A_APP_CUSTOM_PARITY;
                ParityBufferPtr = &CodecBuffer[ISO14443A_BUFFER_PARITY_OFFSET];
            } else {
                /* We have to generate the parity bits ourself */
                ParityBufferPtr = 0;
            }
        }

        if (AnswerBitCount != ISO14443A_APP_NO_RESPONSE) {
            LogEntry(LOG_INFO_CODEC_TX_DATA, CodecBuffer, (AnswerBitCount + 7) / 8);
            LEDHook(LED_CODEC_TX, LED_PULSE);

            BitCount = AnswerBitCount;
            CodecBufferPtr = CodecBuffer;
            CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OOK, ISO14443A_SUBCARRIER_DIVIDER);

            StateRegister = LOADMOD_START;
        } else {
            /* No data to be processed. Disable loadmodding and start listening again */
            CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
            CODEC_TIMER_LOADMOD.INTCTRLA = 0;

            StartDemod();
        }
    }

    if (Flags.LoadmodFinished) {
        Flags.LoadmodFinished = 0;
        /* Load modulation has been finished. Stop it and start to listen
         * for incoming data again. */
        StartDemod();
    }
}

