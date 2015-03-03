/*
 * ISO14443A.c
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#include "ISO14443-2A.h"
#include "../System.h"
#include "../Application/Application.h"
#include "../LED.h"
#include "Codec.h"
#include "Log.h"

/* Timing definitions for ISO14443A */
#define ISO14443A_SUBCARRIER_DIVIDER    16
#define ISO14443A_BIT_GRID_CYCLES       128
#define ISO14443A_BIT_RATE_CYCLES       128
#define ISO14443A_FRAME_DELAY_PREV1     (1236 - 24) /* compensate for ISR prolog */
#define ISO14443A_FRAME_DELAY_PREV0     (1172 - 24)

/* Sampling is done using internal clock, synchronized to the field modulation.
 * For that we need to convert the bit rate for the internal clock. */
#define SAMPLE_RATE_SYSTEM_CYCLES		((uint16_t) (((uint64_t) F_CPU * ISO14443A_BIT_RATE_CYCLES) / CODEC_CARRIER_FREQ) )

static volatile struct {
    volatile bool DemodFinished;
    volatile bool LoadmodFinished;
} Flags = { 0 };

typedef enum {
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
} LoadModStateType;

static volatile uint8_t* CodecBufferPtr;
static volatile uint8_t* ParityBufferPtr;
static volatile uint16_t BitCount;
static volatile uint16_t BitSent;
static volatile uint8_t DataRegister;
static volatile uint8_t SampleRegister;
static volatile bool IsParityBit;
static volatile uint8_t LastBit;
static volatile LoadModStateType LoadModState;
static volatile bool SamplePosition;

static void Initialize(void) {
    /* Configure CARRIER input pin and route it to EVSYS */
    CODEC_CARRIER_IN_PORT.DIRCLR = CODEC_CARRIER_IN_MASK;
    CODEC_CARRIER_IN_PORT.CODEC_CARRIER_IN_PINCTRL = PORT_ISC_BOTHEDGES_gc;
    EVSYS.CH6MUX = CODEC_CARRIER_IN_EVMUX;

    /* Configure two DEMOD pins for input.
     * Configure event channel 0 for rising edge (begin of modulation pause)
     * Configure event channel 1 for falling edge (end of modulation pause) */
    CODEC_DEMOD_IN_PORT.DIRCLR = CODEC_DEMOD_IN_MASK;
    CODEC_DEMOD_IN_PORT.CODEC_DEMOD_IN_PINCTRL0 = PORT_ISC_RISING_gc;
    CODEC_DEMOD_IN_PORT.CODEC_DEMOD_IN_PINCTRL1 = PORT_ISC_FALLING_gc;
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
    CODEC_DEMOD_IN_PORT.INTCTRL = PORT_INT0LVL_HI_gc;
    EVSYS.CH0MUX = CODEC_DEMOD_IN_EVMUX0;
    EVSYS.CH1MUX = CODEC_DEMOD_IN_EVMUX1;

    /* Configure LOADMOD and SUBCARRIER output pins.
     * Disable PSK modulation by setting pin to low. */
    CODEC_LOADMOD_PORT.DIRSET = CODEC_LOADMOD_MASK;
    CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
    CODEC_SUBCARRIER_PORT.DIRSET = CODEC_SUBCARRIER_MASK;
    CODEC_SUBCARRIER_PORT.OUTCLR = CODEC_SUBCARRIER_MASK;

    /* Configure subcarrier generation with 50% DC output using OOK */
    CODEC_SUBCARRIER_TIMER.PER = ISO14443A_SUBCARRIER_DIVIDER - 1;
    CODEC_SUBCARRIER_TIMER.CODEC_SUBCARRIER_CC_OOK = ISO14443A_SUBCARRIER_DIVIDER/2;
    CODEC_SUBCARRIER_TIMER.CTRLB = CODEC_SUBCARRIER_CCEN_OOK | TC_WGMODE_SINGLESLOPE_gc;
}

static void StartDemod(void) {
    /* Activate Power for demodulator */
    CodecSetDemodPower(true);

    CodecBufferPtr = CodecBuffer;
    ParityBufferPtr = &CodecBuffer[ISO14443A_BUFFER_PARITY_OFFSET];
    DataRegister = 0;
    SampleRegister = 0;
    SamplePosition = 0;
    BitCount = 0;
    IsParityBit = false;

    /* Configure sampling-timer free running and sync to first modulation-pause. */
    CODEC_TIMER_SAMPLING.CNT = 0;
    CODEC_TIMER_SAMPLING.PER = SAMPLE_RATE_SYSTEM_CYCLES - 1;
    CODEC_TIMER_SAMPLING.CCA = 0xFFFF; /* CCA Interrupt is not active! */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV1_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH0_gc;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCAINTLVL_HI_gc;

    /* Start looking out for modulation pause via interrupt. */
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
    SystemSleepSetMode(SYSTEM_SMODE_PSAVE);
}

ISR(CODEC_DEMOD_IN_INT0_VECT) {
    /* This is the first edge of the first modulation-pause after StartDemod.
     * Now we have time to start
     * demodulating beginning from one bit-width after this edge. */
     SystemSleepSetMode(SYSTEM_SMODE_IDLE);

    /* Sampling timer has been preset to sample-rate and has automatically synced
     * to THIS first modulation pause. Thus after exactly one bit-width from here,
     * an OVF is generated. We want to start sampling with the next bit and use the
     * XYZBUF mechanism of the xmega to automatically double the sampling rate on the
     * next overflow. For this we have to temporarily deactivate the automatical alignment
     * in order to catch next overflow event for updating the BUF registers.
     * We want to sample the demodulated data stream in the first quarter of the half-bit
     * where the pulsed miller encoded is located. */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_SAMPLING.PERBUF = SAMPLE_RATE_SYSTEM_CYCLES/2 - 1; /* Half bit width */
    CODEC_TIMER_SAMPLING.CCABUF = SAMPLE_RATE_SYSTEM_CYCLES/8 - 10 - 1; /* Compensate for DIGFILT and ISR prolog */

    /* Setup Frame Delay Timer and wire to EVSYS. Frame delay time is
     * measured from last change in RF field, therefore we use
     * the event channel 1 (end of modulation pause) as the restart event.
     * The preliminary frame delay time chosen here is irrelevant, because
     * the correct FDT gets set automatically after demodulation. */
    CODEC_TIMER_LOADMOD.CNT = 0;
    CODEC_TIMER_LOADMOD.PER = 0xFFFF;
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH1_gc;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_EVCH6_gc;

    /* Disable this interrupt */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
}

ISR(CODEC_TIMER_SAMPLING_CCA_VECT) {
    /* This interrupt gets called twice for every bit to sample it. */
    uint8_t SamplePin = CODEC_DEMOD_IN_PORT.IN & CODEC_DEMOD_IN_MASK;
    uint8_t NewSampleRegister;

    /* Shift sampled bit into sampling register and hold a local copy for fast access. */
    NewSampleRegister = SampleRegister << 1;
    NewSampleRegister |= (!SamplePin ? 0x01 : 0x00);
    SampleRegister = NewSampleRegister;

    if (SamplePosition) {
        /* Analyze the sampling register after 2 samples. */
        if ((NewSampleRegister & 0x07) == 0x07) {
            /* No carrier modulation for 3 sample points. EOC! */
            CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
            CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCAIF_bm;

            /* By this time, the FDT timer is aligned to the last modulation
             * edge of the reader. So we disable the auto-synchronization and
             * let it count the frame delay time in the background, and generate
             * an interrupt once it has reached the FDT. */
            CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;

            if (LastBit) {
                CODEC_TIMER_LOADMOD.PER = ISO14443A_FRAME_DELAY_PREV1;
            } else {
                CODEC_TIMER_LOADMOD.PER = ISO14443A_FRAME_DELAY_PREV0;
            }

            LoadModState = LOADMOD_FDT;

            CODEC_TIMER_LOADMOD.INTFLAGS = TC1_OVFIF_bm;
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
            uint8_t BitSample = NewSampleRegister & 0xC;
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

                LastBit = Bit;

                if (!IsParityBit) {
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
                        IsParityBit = true;
                    }

                } else {
                    /* This is a parity bit. Store it */
                    /* TODO: Store parity and prevent overflow */
                    //*ParityBufferPtr++ = Bit;
                    IsParityBit = false;
                }
            } else {
                /* 00 sequence. -> No valid data yet. This also occurs if we just started
                 * sampling and have sampled less than 2 bits yet. Thus ignore. */
            }
        }
    } else {
        /* On odd sample position just sample. */
    }

    SamplePosition = !SamplePosition;

    /* Make sure the sampling timer gets automatically aligned to the
     * modulation pauses by using the RESTART event.
     * This can be understood as a "poor mans PLL" and makes sure that we are
     * never too far out the bit-grid while sampling. */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH0_gc;
}

ISR(CODEC_TIMER_OVF_VECT) {
    /* Bit rate timer. Output a half bit on the output. */
    uint8_t Temp8;
    uint16_t Temp16;

    switch (LoadModState) {
    case LOADMOD_FDT:
        /* No data has been produced, but FDT has ended. Switch over to bit-grid aligning. */
        CODEC_TIMER_LOADMOD.PER = ISO14443A_BIT_GRID_CYCLES - 1;
        break;

    case LOADMOD_START:
        /* Application produced data. With this interrupt we are aligned to the bit-grid.
         * Start subcarrier generation and align to bitrate. */
        CODEC_TIMER_LOADMOD.PER = ISO14443A_BIT_RATE_CYCLES / 2 - 1;
        CODEC_SUBCARRIER_TIMER.CTRLA = TC_CLKSEL_EVCH6_gc;

        /* Fallthrough to first bit */

    case LOADMOD_START_BIT0:
    	CODEC_LOADMOD_PORT.OUTSET = CODEC_LOADMOD_MASK;
        LoadModState = LOADMOD_START_BIT1;
        break;

    case LOADMOD_START_BIT1:
    	CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
        LoadModState = LOADMOD_DATA0;

        /* Fetch first byte */
        DataRegister = *CodecBufferPtr;
        break;

    case LOADMOD_DATA0:
        if (DataRegister & 1) {
        	CODEC_LOADMOD_PORT.OUTSET = CODEC_LOADMOD_MASK;
        } else {
        	CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
        }

        LoadModState = LOADMOD_DATA1;
        break;

    case LOADMOD_DATA1:
        Temp8 = DataRegister;

        if (Temp8 & 1) {
        	CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
        } else {
        	CODEC_LOADMOD_PORT.OUTSET = CODEC_LOADMOD_MASK;
        }

        DataRegister = Temp8 >> 1;

        Temp16 = BitSent;
        BitSent = ++Temp16;

        if ((Temp16 & 0x07) == 0) {
            /* Byte boundary. Load parity bit and output it later. */
            LoadModState = LOADMOD_PARITY0;
            break;
        }

        if (Temp16 == BitCount) {
            /* End of transmission without byte boundary. Don't send parity. */
            LoadModState = LOADMOD_STOP_BIT0;
            break;
        }

        /* Next bit is data */
        LoadModState = LOADMOD_DATA0;

        break;

    case LOADMOD_PARITY0:
        if (*ParityBufferPtr) {
        	CODEC_LOADMOD_PORT.OUTSET = CODEC_LOADMOD_MASK;
        } else {
        	CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
        }

        LoadModState = LOADMOD_PARITY1;
        break;

    case LOADMOD_PARITY1:
        if (*ParityBufferPtr) {
        	CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
        } else {
        	CODEC_LOADMOD_PORT.OUTSET = CODEC_LOADMOD_MASK;
        }

        if (BitSent == BitCount) {
            /* No data left */
            LoadModState = LOADMOD_STOP_BIT0;
        } else {
            /* Fetch next data and continue sending bits. */
            ParityBufferPtr++;
            DataRegister = *++CodecBufferPtr;
            LoadModState = LOADMOD_DATA0;
        }

        break;

    case LOADMOD_STOP_BIT0:
    	CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
        LoadModState = LOADMOD_STOP_BIT1;
        break;

    case LOADMOD_STOP_BIT1:
    	CODEC_LOADMOD_PORT.OUTCLR = CODEC_LOADMOD_MASK;
        LoadModState = LOADMOD_FINISHED;
        break;

    case LOADMOD_FINISHED:
        /* We have written all of our bits. Deactivate the loadmod
         * timer. Also disable the bit-rate interrupt again. And
         * stop the subcarrier divider. */
        CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
        CODEC_TIMER_LOADMOD.INTCTRLA = 0;
        CODEC_SUBCARRIER_TIMER.CTRLA = TC_CLKSEL_OFF_gc;

        /* Signal application that we have finished loadmod */
        Flags.LoadmodFinished = 1;
        break;

    default:
        break;
    }
}

void ISO14443ACodecInit(void) {
    /* Initialize common peripherals and start listening
     * for incoming data. */
    Initialize();
    StartDemod();
}

void ISO14443ACodecTask(void) {
    if (Flags.DemodFinished) {
        Flags.DemodFinished = 0;
        /* Reception finished. Process the received bytes */
        CodecSetDemodPower(false);

        uint16_t DemodBitCount = BitCount;
        uint16_t AnswerBitCount = ISO14443A_APP_NO_RESPONSE;

        if (DemodBitCount > 0) {
            LogEntry(LOG_INFO_RX_DATA, CodecBuffer, (DemodBitCount+7)/8);
            LEDTrigger(LED_CODEC_RX, LED_PULSE);

            /* Call application if we received data */
            AnswerBitCount = ApplicationProcess(CodecBuffer, DemodBitCount);

            if (AnswerBitCount & ISO14443A_APP_CUSTOM_PARITY) {
                /* Application has generated it's own parity bits.
                 * Clear this option bit. */
                AnswerBitCount &= ~ISO14443A_APP_CUSTOM_PARITY;
            } else {
                /* We have to generate the parity bits ourself */
                for (uint8_t i = 0; i < (AnswerBitCount / 8); i++) {
                    /* For each whole byte, generate a parity bit. */
                    CodecBuffer[ISO14443A_BUFFER_PARITY_OFFSET + i] =
                            ODD_PARITY(CodecBuffer[i]);
                }
            }
        } else {
            ApplicationReset();
        }

        if (AnswerBitCount != ISO14443A_APP_NO_RESPONSE) {
            LogEntry(LOG_INFO_TX_DATA, CodecBuffer, (AnswerBitCount + 7) / 8);
            LEDTrigger(LED_CODEC_TX, LED_PULSE);

            BitCount = AnswerBitCount;
            BitSent = 0;
            CodecBufferPtr = CodecBuffer;
            ParityBufferPtr = &CodecBuffer[ISO14443A_BUFFER_PARITY_OFFSET];
            LoadModState = LOADMOD_START;
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

    //SystemSleep();
}

