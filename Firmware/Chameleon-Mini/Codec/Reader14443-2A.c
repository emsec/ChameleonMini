/*
 * Reader14443-2A.c
 *
 *  Created on: 26.08.2014
 *      Author: sk
 */

#include "Reader14443-2A.h"
#include "Codec.h"
#include "../System.h"
#include "../Application/Application.h"
#include "LEDHook.h"
#include "Terminal/Terminal.h"
#include <util/delay.h>

#define SAMPLE_RATE_SYSTEM_CYCLES		((uint16_t) (((uint64_t) F_CPU * ISO14443A_BIT_RATE_CYCLES) / CODEC_CARRIER_FREQ) )
#define ISO14443A_RX_MINIMUM_BITCOUNT	4
#define ISO14443A_PICC_TO_PCD_FDT_PRESCALER	TC_CLKSEL_DIV8_gc // please change ISO14443A_PICC_TO_PCD_MIN_FDT when changing this
#define ISO14443A_PICC_TO_PCD_MIN_FDT	293

static volatile struct {
    volatile bool Start;
    volatile bool RxDone;
    volatile bool RxPending;
} Flags = { 0 };

static volatile uint16_t RxPendingSince;

static volatile enum {
    STATE_IDLE,
    STATE_MILLER_SEND,
    STATE_MILLER_EOF,
    STATE_FDT
} State;

// GPIOR0 and 1 are used as storage for the timer value of the current modulation
#define LastBit			Codec8Reg2				// GPIOR2
// GPIOR3 is used for some internal flags
#define BitCount		CodecCount16Register1	// GPIOR5:4
#define SampleRegister	GPIOR6
#define BitCountUp		GPIOR7
#define CodecBufferIdx	GPIOR8
#define CodecBufferPtr	CodecPtrRegister2

#define UINT8DIFF(a,b) ((uint8_t) (a-b))

void Reader14443ACodecInit(void) {
    /* Initialize common peripherals and start listening
     * for incoming data. */
    CodecInitCommon();
    isr_func_TCD0_CCC_vect = &isr_Reader14443_2A_TCD0_CCC_vect;
    CodecSetDemodPower(true);

    CODEC_TIMER_SAMPLING.PER = SAMPLE_RATE_SYSTEM_CYCLES - 1;
    CODEC_TIMER_SAMPLING.CCB = 0;
    CODEC_TIMER_SAMPLING.CCC = 0;
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLA = 0;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCBINTLVL_OFF_gc;
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV1_gc;

    CODEC_TIMER_LOADMOD.CTRLA = 0;
    State = STATE_IDLE;

    Flags.Start = false;
    Flags.RxPending = false;
    Flags.RxDone = false;
}

void Reader14443ACodecDeInit(void) {
    CodecSetDemodPower(false);
    CodecReaderFieldStop();
    CODEC_TIMER_SAMPLING.CTRLA = 0;
    CODEC_TIMER_SAMPLING.INTCTRLB = 0;
    CODEC_TIMER_LOADMOD.CTRLA = 0;
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;
    Flags.RxDone = false;
    Flags.RxPending = false;
    Flags.Start = false;
}

INLINE void Insert0(void) {
    SampleRegister >>= 1;
    if (++BitCount % 8)
        return;
    *CodecBufferPtr++ = SampleRegister;
}

INLINE void Insert1(void) {
    SampleRegister = (SampleRegister >> 1) | 0x80;
    if (++BitCount % 8)
        return;
    *CodecBufferPtr++ = SampleRegister;
}


// End of Card-> reader communication and enter frame delay time
INLINE void Reader14443A_EOC(void) {
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = 0;
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_OFF_gc;
    ACA.AC1CTRL &= ~AC_ENABLE_bm;

    if (BitCount & 1) {
        if (SampleRegister & 0x80)
            Insert0();
        else
            Insert1();
    }

    if (BitCount % 8) // copy the last byte, if there is an incomplete byte
        CodecBuffer[BitCount / 8] = SampleRegister >> (8 - (BitCount % 8));
    Flags.RxDone = true;
    Flags.RxPending = false;

    // set up timer that forces the minimum frame delay time from PICC to PCD
    CODEC_TIMER_LOADMOD.PER = 0xFFFF;
    CODEC_TIMER_LOADMOD.CNT = 0;
    CODEC_TIMER_LOADMOD.INTCTRLA = 0;
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;
    CODEC_TIMER_LOADMOD.CTRLD = 0;
    CODEC_TIMER_LOADMOD.CTRLA = ISO14443A_PICC_TO_PCD_FDT_PRESCALER;

    State = STATE_FDT;
}

INLINE void BufferToSequence(void) {
    uint16_t count = BitCount;
    if (count > BITS_PER_BYTE * CODEC_BUFFER_SIZE / 2) // todo is this correct?
        return;

    BitCount = 0;

    memcpy(CodecBuffer + CODEC_BUFFER_SIZE / 2, CodecBuffer, (count + 7) / 8);
    uint8_t *Buffer = CodecBuffer + CODEC_BUFFER_SIZE / 2;
    CodecBufferPtr = CodecBuffer;

    // Modified Miller Coding ISO14443-2 8.1.3
    Insert1(); // SOC
    Insert0();

    uint16_t i;
    uint8_t last = 0;
    for (i = 1; i <= count; i++) {
        if ((*Buffer) & 1) {
            Insert0();
            Insert1();
            last = 1;
        } else {
            if (last) {
                Insert0();
                Insert0();
            } else {
                Insert1();
                Insert0();
            }
            last = 0;
        }
        *Buffer >>= 1;
        if ((i % 8) == 0)
            Buffer++;
    }

    if (last == 0) { // EOC
        Insert1();
        Insert0();
    }

    if (BitCount % 8)
        CodecBuffer[BitCount / 8] = SampleRegister >> (8 - (BitCount % 8));
}

// ISR (TCD0_CCC_vect)
// Frame Delay Time PCD to PICC ends
ISR_SHARED isr_Reader14443_2A_TCD0_CCC_vect(void) {
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCCIF_bm;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCCINTLVL_OFF_gc;

    /* Enable the AC interrupt, which either finds the SOC and then starts the pause-finding timer,
     * or it is triggered before the SOC, which mostly isn't bad at all, since the first pause
     * needs to be found. */
    ACA.STATUS = AC_AC1IF_bm;
    ACA.AC1CTRL = AC_HSMODE_bm | AC_HYSMODE_NO_gc | AC_INTMODE_FALLING_gc | AC_INTLVL_HI_gc | AC_ENABLE_bm;

    CodecBufferPtr = CodecBuffer; // use GPIOR for faster access
    BitCount = 1; // FALSCH todo the first modulation of the SOC is "found" implicitly
    SampleRegister = 0x00;

    RxPendingSince = SystemGetSysTick();
    Flags.RxPending = true;

    // reset for future use
    CodecBufferIdx = 0;
    BitCountUp = 0;

    State = STATE_IDLE;
    PORTE.OUTTGL = PIN3_bm;
}

// Reader -> card send bits finished
// Start Frame delay time PCD to PICC
void Reader14443AMillerEOC(void) {
    CODEC_TIMER_SAMPLING.PER = 5 * SAMPLE_RATE_SYSTEM_CYCLES - 1;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCBIF_bm | TC0_CCCIF_bm;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCBINTLVL_OFF_gc | TC_CCCINTLVL_HI_gc;
    CODEC_TIMER_SAMPLING.PERBUF = SAMPLE_RATE_SYSTEM_CYCLES - 1;
    PORTE.OUTTGL = PIN3_bm;
}

// EOC of Card->Reader found
ISR(CODEC_TIMER_TIMESTAMPS_CCA_VECT) { // EOC found
    Reader14443A_EOC();
}

// This interrupt find Card -> Reader SOC
ISR(ACA_AC1_vect) { // this interrupt either finds the SOC or gets triggered before
    ACA.AC1CTRL &= ~AC_INTLVL_HI_gc; // disable this interrupt
    // enable the pause-finding timer
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH0_gc;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_DIV1_gc;
}

// Decode the Card -> Reader signal
// according to the pause and modulated period
// if the half bit duration is modulated, then add 1 to buffer
// if the half bit duration is not modulated, then add 0 to buffer
ISR(CODEC_TIMER_LOADMOD_CCA_VECT) { // pause found
    uint8_t tmp = CODEC_TIMER_TIMESTAMPS.CNTL;
    CODEC_TIMER_TIMESTAMPS.CNT = 0;

    /* This needs to be done only on the first call,
     * but doing this only on a condition means wasting time, so we do it every time. */
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_DIV4_gc;

    switch (tmp) { // decide how many half bit periods have been modulations
        case 0 ... 48: // 32 ticks is one half of a bit period
            return;

        case 49 ... 80: // 64 ticks are a full bit period
            Insert1();
            Insert0();
            return;

        case 81 ... 112: // 96 ticks are 3 half bit periods
            if (BitCount & 1) {
                Insert1();
                Insert1();
                Insert0();
            } else {
                Insert1();
                Insert0();
                Insert0();
            }
            return;

        default: // every value over 96 + 16 (tolerance) is considered to be 4 half bit periods
            Insert1();
            Insert1();
            Insert0();
            Insert0();
            return;
    }
    return;
}

void Reader14443ACodecTask(void) {
    if (Flags.RxPending && SYSTICK_DIFF(RxPendingSince) > Reader_FWT + 1) {
        Reader14443A_EOC();
        BitCount = 0;
        Flags.RxDone = true;
        Flags.RxPending = false;
    }
    if (CodecIsReaderToBeRestarted() || !CodecIsReaderFieldReady())
        return;
    if (!Flags.RxPending && (Flags.Start || Flags.RxDone)) {
        if (State == STATE_FDT && CODEC_TIMER_LOADMOD.CNT < ISO14443A_PICC_TO_PCD_MIN_FDT) // we are in frame delay time, so we can return later
            return;
        if (Flags.RxDone && BitCount > 0) { // decode the raw received data
            if (BitCount < ISO14443A_RX_MINIMUM_BITCOUNT * 2) {
                BitCount = 0;
            } else {
                uint8_t TmpCodecBuffer[CODEC_BUFFER_SIZE];
                memcpy(TmpCodecBuffer, CodecBuffer, (BitCount + 7) / 8);

                CodecBufferPtr = CodecBuffer;
                uint16_t BitCountTmp = 2, TotalBitCount = BitCount;
                BitCount = 0;

                bool breakflag = false;
                TmpCodecBuffer[0] >>= 2; // with this (and BitCountTmp = 2), the SOC is ignored

                // Manchester Code ISO14443-2 8.2.5
                while (!breakflag && BitCountTmp < TotalBitCount) {
                    uint8_t Bit = TmpCodecBuffer[BitCountTmp / 8] & 0x03;
                    TmpCodecBuffer[BitCountTmp / 8] >>= 2;
                    switch (Bit) {
                        case 0b10:
                            Insert1();
                            break;

                        case 0b01:
                            Insert0();
                            break;

                        case 0b00: // EOC
                            breakflag = true;
                            break;

                        default:
                            // error, should not happen, TODO handle this
                            break;
                    }
                    BitCountTmp += 2;
                }
                if (BitCount % 8) // copy the last byte, if there is an incomplete byte
                    CodecBuffer[BitCount / 8] = SampleRegister >> (8 - (BitCount % 8));
                LEDHook(LED_CODEC_RX, LED_PULSE);
                LogEntry(LOG_INFO_CODEC_RX_DATA_W_PARITY, CodecBuffer, (BitCount + 7) / 8);
            }
        }
        Flags.Start = false;
        Flags.RxDone = false;

        /* Call application with received data */
        BitCount = ApplicationProcess(CodecBuffer, BitCount);

        if (BitCount > 0) {
            /*
             * Prepare for Manchester decoding.
             * The basic idea is to use two timers. The first one will be reset everytime the DEMOD signal
             * passes a (configurable) threshold. This is realized with the event system and an analog
             * comparator.
             * Once this timer reaches 3/4 of a bit half (this means it has not been reset this long), we
             * assume there is a pause. Now we read the second timers count value and can decide how many
             * bit halves had modulations since the last pause.
             */

            /* Configure and enable the analog comparator for finding pauses in the DEMOD signal. */
            ACA.AC1CTRL = AC_HSMODE_bm | AC_HYSMODE_NO_gc | AC_INTMODE_FALLING_gc | AC_ENABLE_bm;

            /* This timer will be used to detect the pauses between the modulation sequences. */
            CODEC_TIMER_LOADMOD.CTRLA = 0;
            CODEC_TIMER_LOADMOD.CNT = 0;
            CODEC_TIMER_LOADMOD.PER = 0xFFFF; // with 27.12 MHz this is exactly one half bit width
            CODEC_TIMER_LOADMOD.CCA = 95; // with 27.12 MHz this is 3/4 of a half bit width
            CODEC_TIMER_LOADMOD.INTCTRLA = 0;
            CODEC_TIMER_LOADMOD.INTFLAGS = TC1_CCAIF_bm;
            CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCAINTLVL_HI_gc;

            /* This timer will be used to find out how many bit halfs since the last pause have been passed. */
            CODEC_TIMER_TIMESTAMPS.CNT = 0;
            CODEC_TIMER_TIMESTAMPS.PER = 0xFFFF;
            CODEC_TIMER_TIMESTAMPS.CCA = 160;
            CODEC_TIMER_TIMESTAMPS.INTCTRLA = 0;
            CODEC_TIMER_TIMESTAMPS.INTFLAGS = TC1_CCAIF_bm;
            CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCAINTLVL_LO_gc;

            /* Use the event system for resetting the pause-detecting timer. */
            EVSYS.CH0MUX = EVSYS_CHMUX_ACA_CH1_gc; // on every ACA_AC1 INT
            EVSYS.CH0CTRL = EVSYS_DIGFILT_1SAMPLE_gc;

            ACA.AC0CTRL = 0;
            CODEC_DEMOD_IN_PORT.INTCTRL = 0;

            LEDHook(LED_CODEC_TX, LED_PULSE);
            LogEntry(LOG_INFO_CODEC_TX_DATA_W_PARITY, CodecBuffer, (BitCount + 7) / 8);

            /* Set state and start timer for Miller encoding. */
            // Send bits to card using TCD0_CCB interrupt (See Reader14443-ISR.S)
            BufferToSequence();
            State = STATE_MILLER_SEND;
            CodecBufferPtr = CodecBuffer;
            CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCBIF_bm;
            CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCBINTLVL_HI_gc;
            _delay_loop_1(85);
        }
    }
}

void Reader14443ACodecStart(void) {
    /* Application wants us to start a card transaction */
    BitCount = 0;
    Flags.Start = true;

    CodecReaderFieldStart();
}

void Reader14443ACodecReset(void) {
    Reader14443A_EOC(); // this breaks every interrupt etc.
    State = STATE_IDLE;
    Flags.RxDone = false;
    Flags.Start = false;
    CodecReaderFieldStop();
}
