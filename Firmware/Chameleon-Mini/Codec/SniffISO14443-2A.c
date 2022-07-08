//
// Created by Zitai Chen on 05/07/2018.
// Sniffing Function of ISO14443-2A cards
// Wors in both direction: PCD->PICC, PICC->PCD,
// modified from ISO14443-2A.c and Reader14443-2A.c
//

#include "SniffISO14443-2A.h"

#include "Reader14443-2A.h"
#include "Codec.h"
#include "../System.h"
#include "../Application/Application.h"
#include "LEDHook.h"
#include "Terminal/Terminal.h"
#include <util/delay.h>

/* Sampling is done using internal clock, synchronized to the field modulation.
 * For that we need to convert the bit rate for the internal clock. */
#define SAMPLE_RATE_SYSTEM_CYCLES		    ((uint16_t) (((uint64_t) F_CPU * ISO14443A_BIT_RATE_CYCLES) / CODEC_CARRIER_FREQ) )
#define ISO14443A_MIN_BITS_PER_FRAME		7

// Card to reader
#define ISO14443A_PICC_TO_PCD_FDT_PRESCALER	TC_CLKSEL_DIV8_gc // please change ISO14443A_PICC_TO_PCD_MIN_FDT when changing this
#define ISO14443A_RX_MINIMUM_BITCOUNT	    4

/* Define pseudo variables to use fast register access. This is useful for global vars */
#define DataRegister	Codec8Reg0
#define StateRegister	Codec8Reg1
#define ParityRegister	Codec8Reg2

#define SampleIdxRegister Codec8Reg3
#define ReaderSampleR	GPIOR4
#define CardSampleR     GPIOR5
#define BitCount		CodecCount16Register2
#define ReaderBufferPtr	CodecPtrRegister1
#define ParityBufferPtr	CodecPtrRegister2
#define CardBufferPtr   CodecPtrRegister3

static volatile struct {
    volatile bool ReaderDataAvaliable;
    volatile bool CardDataAvaliable;

} Flags = { 0 };
static volatile uint16_t RxPendingSince;

typedef enum {
    DEMOD_DATA_BIT,    /* Demod */
    DEMOD_PARITY_BIT,
    PCD_PICC_FDT,
    PICC_FRAME,

} StateType;

static volatile uint16_t ReaderBitCount;
static volatile uint16_t CardBitCount;
static volatile uint16_t rawBitCount;

INLINE void CardSniffInit(void);
INLINE void CardSniffDeinit(void);

/////////////////////////////////////////////////
// Reader->Card Direction Traffic
/////////////////////////////////////////////////

INLINE void ReaderSniffInit(void) {
//    PORTE.OUTSET = PIN3_bm;

    // Configure interrupt for demod
    CODEC_DEMOD_IN_PORT.INTCTRL = PORT_INT1LVL_HI_gc;

    /* Initialize some global vars and start looking out for reader commands */

    ReaderBufferPtr = CodecBuffer;
    ParityBufferPtr = &CodecBuffer[ISO14443A_BUFFER_PARITY_OFFSET];
    DataRegister = 0;
    ReaderSampleR = 0;
    SampleIdxRegister = 0;
    BitCount = 0;
    StateRegister = DEMOD_DATA_BIT;


    /* Configure sampling-timer free running and sync to first modulation-pause. */
    CODEC_TIMER_SAMPLING.CNT = 0;                               // Reset the timer count
    CODEC_TIMER_SAMPLING.PER = SAMPLE_RATE_SYSTEM_CYCLES - 1;   // Set Period regisiter
    CODEC_TIMER_SAMPLING.CCD = 0xFFFF; /* CCD Interrupt is not active! */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV1_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCDIF_bm;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCDINTLVL_HI_gc;

    /* Start looking out for modulation pause via interrupt. */
    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT1IF_bm;
    CODEC_DEMOD_IN_PORT.INT1MASK = CODEC_DEMOD_IN_MASK0;
}
INLINE void ReaderSniffDeInit(void) {
//    PORTE.OUTCLR = PIN3_bm;

    /* Gracefully shutdown codec */
    CODEC_DEMOD_IN_PORT.INT1MASK = 0;
    CODEC_DEMOD_IN_PORT.INTCTRL = 0;

    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCDINTLVL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCDIF_bm;
}


// Find first pause and start sampling
ISR(CODEC_DEMOD_IN_INT1_VECT) {
    PORTE.OUTSET = PIN2_bm;

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
    CODEC_TIMER_SAMPLING.CCDBUF = SAMPLE_RATE_SYSTEM_CYCLES / 8 - 14 - 1; /* Compensate for DIGFILT and ISR prolog */

    /* Disable this interrupt */
    CODEC_DEMOD_IN_PORT.INT1MASK = 0;
}

// Sampling with timer and demod
ISR(CODEC_TIMER_SAMPLING_CCD_VECT) {
    /* This interrupt gets called twice for every bit to sample it. */
    uint8_t SamplePin = CODEC_DEMOD_IN_PORT.IN & CODEC_DEMOD_IN_MASK;

    /* Shift sampled bit into sampling register */
    ReaderSampleR = (ReaderSampleR << 1) | (!SamplePin ? 0x01 : 0x00);

    if (SampleIdxRegister) {
        SampleIdxRegister = 0;
        /* Analyze the sampling register after 2 samples. */
        if ((ReaderSampleR & 0x07) == 0x07) {
            /* No carrier modulation for 3 sample points. EOC! */

            // Shutdown the Reader->Card Sniffing,
            // disable the sampling timer
            PORTE.OUTCLR = PIN2_bm;



            CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
            CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCDIF_bm;
            CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCDINTLVL_OFF_gc;
            CODEC_DEMOD_IN_PORT.INTCTRL = 0;                        // Disable CODEC_DEMOD_IN_PORT interrupt
            // CTRLD already disabled in CODEC_DEMOD_IN_INT1_VECT,
            // so no need to disable it again it here

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
                *ReaderBufferPtr = NewDataRegister;
            }

            /* Signal, that we have finished sampling */
            ReaderBitCount = BitCount;

            // If we are have got data
            // Start Card->Reader Sniffing without waiting for the complete of CodecTask
            // Otherwise some bit will not be captured
            if (ReaderBitCount >= ISO14443A_MIN_BITS_PER_FRAME) {
                Flags.ReaderDataAvaliable = true;
                CardSniffInit();
            } else {
                ReaderSniffInit();
            }

            return;

        } else {
            /* Otherwise, we check the two sample bits from the bit before. */
            uint8_t BitSample = ReaderSampleR & 0xC;
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
                        *ReaderBufferPtr++ = NewDataRegister;

                        /* Store bit for determining FDT at EOC and enable parity
                         * handling on next bit. */
                        StateRegister = DEMOD_PARITY_BIT;
                    }

                } else if (StateRegister == DEMOD_PARITY_BIT) {
                    /* This is a parity bit. Store it */
//                    *ParityBufferPtr++ = Bit;
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

/////////////////////////////////////////////////
// Card->Reader Direction Traffic
/////////////////////////////////////////////////

INLINE void CardSniffInit(void) {

    /* Initialize common peripherals and start listening
     * for incoming data. */

    CardBufferPtr = CodecBuffer2; // use GPIOR for faster access
    rawBitCount = 1; // FALSCH todo the first modulation of the SOC is "found" implicitly
    BitCount = 0;
    CardSampleR = 0x00;


    /*
     * Prepare for Manchester decoding.
     * The basic idea is to use two timers. The first one will be reset everytime the DEMOD signal
     * passes a (configurable) threshold. This is realized with the event system and an analog
     * comparator.
     * Once this timer reaches 3/4 of a bit half (this means it has not been reset this long), we
     * assume there is a pause. Now we read the second timers count value and can decide how many
     * bit halves had modulations since the last pause.
     */

    // Comparator ADC
    /* Configure and enable the analog comparator for finding pauses in the DEMOD signal. */
    ACA.AC0CTRL = AC_HSMODE_bm | AC_HYSMODE_NO_gc | AC_INTMODE_FALLING_gc | AC_ENABLE_bm;

    /* This timer will be used to detect the pauses between the modulation sequences. */
    CODEC_TIMER_LOADMOD.CTRLA = 0;
    CODEC_TIMER_LOADMOD.CNT = 0;
    CODEC_TIMER_LOADMOD.PER = 0xFFFF; // with 27.12 MHz this is exactly one half bit width
    CODEC_TIMER_LOADMOD.CCB = 95; // with 27.12 MHz this is 3/4 of a half bit width
    CODEC_TIMER_LOADMOD.INTCTRLA = 0;
    CODEC_TIMER_LOADMOD.INTFLAGS = TC1_CCBIF_bm;
    CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCBINTLVL_HI_gc;

    /* This timer will be used to find out how many bit halfs since the last pause have been passed. */
    CODEC_TIMER_TIMESTAMPS.CNT = 0;                         // Reset timer
    CODEC_TIMER_TIMESTAMPS.PER = 0xFFFF;
    CODEC_TIMER_TIMESTAMPS.CCB = 160;
    CODEC_TIMER_TIMESTAMPS.INTCTRLA = 0;
    CODEC_TIMER_TIMESTAMPS.INTFLAGS = TC1_CCBIF_bm;         // Clear interrupt flag
//    CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCBINTLVL_LO_gc;
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCBINTLVL_HI_gc;

    /* Use the event system for resetting the pause-detecting timer. */
    EVSYS.CH2MUX = EVSYS_CHMUX_ACA_CH0_gc; // on every ACA_AC0 INT
    EVSYS.CH2CTRL = EVSYS_DIGFILT_1SAMPLE_gc;

    /* Enable the AC interrupt, which either finds the SOC and then starts the pause-finding timer,
     * or it is triggered before the SOC, which mostly isn't bad at all, since the first pause
     * needs to be found. */
    ACA.AC1CTRL = 0;
    ACA.STATUS = AC_AC0IF_bm;
    ACA.AC0CTRL = AC_HSMODE_bm | AC_HYSMODE_NO_gc | AC_INTMODE_FALLING_gc | AC_INTLVL_HI_gc | AC_ENABLE_bm;

    RxPendingSince = SystemGetSysTick();
    StateRegister = PCD_PICC_FDT;


}

INLINE void CardSniffDeinit(void) {

    CODEC_TIMER_LOADMOD.CTRLA = 0;
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;

    // Disable event system CH2
    EVSYS.CH2MUX = 0;
    EVSYS.CH2CTRL = 0;
    // Reset ACA AC0 to default setting
    ACA.AC0MUXCTRL = AC_MUXPOS_DAC_gc | AC_MUXNEG_PIN7_gc;
    ACA.AC0CTRL = CODEC_AC_DEMOD_SETTINGS;

}



INLINE void Insert0(void) {
    CardSampleR >>= 1;
    if (++BitCount % 8)
        return;
    *CardBufferPtr++ = CardSampleR;
}

INLINE void Insert1(void) {
    CardSampleR = (CardSampleR >> 1) | 0x80;
    if (++BitCount % 8)
        return;
    *CardBufferPtr++ = CardSampleR;
}

// This interrupt find Card -> Reader SOC
ISR_SHARED isr_SniffISO14443_2A_ACA_AC0_VECT(void) {  // this interrupt either finds the SOC or gets triggered before
    ACA.AC0CTRL &= ~AC_INTLVL_HI_gc; // disable this interrupt
    // enable the pause-finding timer
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH2_gc;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_DIV1_gc;
    StateRegister = PICC_FRAME;
}

// Called once a pause is found
// Decode the Card -> Reader signal
// according to the pause and modulated period
// if the half bit duration is modulated, then add 1 to buffer
// if the half bit duration is not modulated, then add 0 to buffer
//ISR(CODEC_TIMER_LOADMOD_CCB_VECT) // pause found
ISR_SHARED isr_SniffISO14443_2A_CODEC_TIMER_LOADMOD_CCB_VECT(void) {
    uint8_t tmp = CODEC_TIMER_TIMESTAMPS.CNTL;
    CODEC_TIMER_TIMESTAMPS.CNT = 0;

    /* This needs to be done only on the first call,
     * but doing this only on a condition means wasting time, so we do it every time. */
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_DIV4_gc;

    // Remember, LSB is send first
    // If current raw bit count is odd, then the previous raw bit must be 0
    switch (tmp) { // decide how many half bit periods have been modulations
        case 0 ... 48: // 32 ticks is one half of a bit period
            return;

        case 49 ... 80: // 64 ticks are a full bit period
            // Got 01
            if (rawBitCount & 1) {
                // 01 + 0 -> 0 10
                // 10 -> 1, last 0 is ignored
                if (rawBitCount > 1) {
                    // Ignore SOC
                    Insert1();
                }
            } else {
                // Current sampled bit count is even, decode directly
                // 01 -> 0
                Insert0();
            }
            rawBitCount += 2;
            return;

        case 81 ... 112: // 96 ticks are 3 half bit periods
            if (rawBitCount & 1) {
                // Current sampled bit count is odd
                // Got 011
                // 011 + 0 -> 01 10 -> 01
                if (rawBitCount > 1) {
                    // Ignore SOC
                    Insert1();
                }
                Insert0();
            } else {
                // Even bit count
                // Got 001
                // 001 -> 0 01, The last 0 is ignored
                // 01 -> 0
                Insert0();
            }
            rawBitCount += 3;

            return;

        default: // every value over 96 + 16 (tolerance) is considered to be 4 half bit periods
            // Got 00 11
            if (rawBitCount & 1) {
                // 00 11 + 0 -> 0 01 10
                // 01 -> 0, 10 -> 1, Ignore last 0
                if (rawBitCount > 1) {
                    // Ignore SOC
                    Insert1();
                }
                Insert0();
            } else {
                // Should not happen
                // If modulation is correct,
                // there should not be a full bit period modulation in even bit count
            }
            rawBitCount += 4;

            return;
    }
}
// EOC of Card->Reader found
ISR(CODEC_TIMER_TIMESTAMPS_CCB_VECT) { // EOC found

    // Disable LOADMOD Timer
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;               // Disable Interrupt
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;   // Disable Clock
    CODEC_TIMER_LOADMOD.CTRLD = 0;                  // Disable connection to event channel

    CODEC_TIMER_TIMESTAMPS.INTCTRLB = 0;
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_OFF_gc;
    ACA.AC0CTRL &= ~AC_ENABLE_bm;

    // If finished in odd sample count
    // There must been an incomplete decoded bit
    // Since only EOC is no modulation in full bit period, and previous raw bit must be 0
    // so the last not modulated bit must be 1
    if (rawBitCount & 1) {
        // The previous raw bit must be 0
        // 1 + 0 -> 10 -> 1
        Insert1();
    }

    if (BitCount % 8) // copy the last byte, if there is an incomplete byte
        CodecBuffer2[BitCount / 8] = CardSampleR >> (8 - (BitCount % 8));

    CardBitCount = BitCount;
    if (BitCount >= ISO14443A_RX_MINIMUM_BITCOUNT) {
        Flags.CardDataAvaliable = true;
    }

    CardSniffDeinit();
    ReaderSniffInit();

}

/////////////////////////////////////////////////
// Init and deInit, task, functions for this codec
/////////////////////////////////////////////////

void Sniff14443ACodecInit(void) {

    PORTE.DIRSET = PIN3_bm | PIN2_bm;
    // Common Codec Register settings
    CodecInitCommon();
    isr_func_ACA_AC0_vect = &isr_SniffISO14443_2A_ACA_AC0_VECT;
    isr_func_CODEC_TIMER_LOADMOD_CCB_VECT = &isr_SniffISO14443_2A_CODEC_TIMER_LOADMOD_CCB_VECT;
    // Enable demodulator power
    CodecSetDemodPower(true);

    // Start with sniffing Reader->Card direction traffic
    Flags.ReaderDataAvaliable = false;
    Flags.CardDataAvaliable = false;

    ReaderSniffInit();
}

void Sniff14443ACodecDeInit(void) {
//    SniffEnable = false;
    CardSniffDeinit();
    ReaderSniffDeInit();
    CodecSetDemodPower(false);
}


void Sniff14443ACodecTask(void) {
    PORTE.OUTSET = PIN3_bm;
    if (Flags.ReaderDataAvaliable) {
        Flags.ReaderDataAvaliable = false;

        LogEntry(LOG_INFO_CODEC_SNI_READER_DATA, CodecBuffer, (ReaderBitCount + 7) / 8);
        // Let the Application layer know where this data comes from
        LEDHook(LED_CODEC_RX, LED_PULSE);

        SniffTrafficSource = TRAFFIC_READER;
        ApplicationProcess(CodecBuffer, ReaderBitCount);
    }


    if (Flags.CardDataAvaliable) {
        Flags.CardDataAvaliable = false;

//        CardBitCount = removeParityBits(CodecBuffer2,CardBitCount );
        LogEntry(LOG_INFO_CODEC_SNI_CARD_DATA_W_PARITY, CodecBuffer2, (CardBitCount + 7) / 8);
        LEDHook(LED_CODEC_RX, LED_PULSE);

        // Let the Application layer know where this data comes from
        SniffTrafficSource = TRAFFIC_CARD;
        ApplicationProcess(CodecBuffer2, CardBitCount);
    }


    if (StateRegister == PCD_PICC_FDT && (SYSTICK_DIFF(RxPendingSince) > Reader_FWT)) {
        CardSniffDeinit();
        ReaderSniffInit();
    }
    PORTE.OUTCLR = PIN3_bm;


}
