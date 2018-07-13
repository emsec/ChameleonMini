//
// Created by Zitai Chen on 05/07/2018.
// Some interrupt handling vect
// modified form ISO14443-2A.c and Reader14443-2A.c
//

// TODO: implement pending since to automatically reset the sniffing after a period of no traffic
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
#define SAMPLE_RATE_SYSTEM_CYCLES		((uint16_t) (((uint64_t) F_CPU * ISO14443A_BIT_RATE_CYCLES) / CODEC_CARRIER_FREQ) )

#define ISO14443A_MIN_BITS_PER_FRAME		7

// Card to reader
#define ISO14443A_PICC_TO_PCD_FDT_PRESCALER	TC_CLKSEL_DIV8_gc // please change ISO14443A_PICC_TO_PCD_MIN_FDT when changing this
#define ISO14443A_RX_MINIMUM_BITCOUNT	4


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


#define BitCountUp		GPIOR7
#define CodecBufferIdx	GPIOR8

enum RCTraffic TrafficSource;

static volatile struct {
    volatile bool DemodFinished;
//    volatile bool LoadmodFinished;
    volatile bool Start;
    volatile bool RxDone;
    volatile bool RxPending;

} Flags = { 0 };
static volatile uint16_t RxPendingSince;


typedef enum {
    /* Demod */
            DEMOD_DATA_BIT,
    DEMOD_PARITY_BIT,
    LOADMOD_FDT,

} StateType;


static volatile enum {
    STATE_IDLE,
    STATE_MILLER_SEND,
    STATE_MILLER_EOF,
    STATE_FDT
} State;

/////////////////////////////////////////////////
// Reader->Card Direction Traffic
/////////////////////////////////////////////////

void ReaderSniffInit(void)
{

    // Configure interrupt for demod
    // This was disabled in CardSniffInit()
    CODEC_DEMOD_IN_PORT.INTCTRL = PORT_INT1LVL_HI_gc;

    TrafficSource = TRAFFIC_READER;

    /* Initialize some global vars and start looking out for reader commands */
    Flags.DemodFinished = 0;

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
    CODEC_TIMER_SAMPLING.CCD = 0xFFFF; /* CCD Interrupt is not active! */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV1_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODSTART_EVSEL;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCDIF_bm;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCDINTLVL_HI_gc;

    /* Start looking out for modulation pause via interrupt. */
    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT1IF_bm;
    CODEC_DEMOD_IN_PORT.INT1MASK = CODEC_DEMOD_IN_MASK0;

//    ISO14443ACodecInit();
}


// Find first pause and start sampling
ISR(CODEC_DEMOD_IN_INT1_VECT) {
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
    CODEC_TIMER_SAMPLING.PERBUF = SAMPLE_RATE_SYSTEM_CYCLES/2 - 1; /* Half bit width */
    CODEC_TIMER_SAMPLING.CCDBUF = SAMPLE_RATE_SYSTEM_CYCLES/8 - 14 - 1; /* Compensate for DIGFILT and ISR prolog */

    /* Disable this interrupt */
    CODEC_DEMOD_IN_PORT.INT1MASK = 0;
}

// Sampling with timer and demod
ISR(CODEC_TIMER_SAMPLING_CCD_VECT) {
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
            CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCDIF_bm;

            // FDT timer (CODEC_TIMER_LOADMOD) is diabled

            StateRegister = LOADMOD_FDT;

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




INLINE void ReaderSniffDeInit(void)
{
    /* Gracefully shutdown codec */
    CODEC_DEMOD_IN_PORT.INT1MASK = 0;

    Flags.DemodFinished = 0;

    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCDINTLVL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCDIF_bm;
}



/////////////////////////////////////////////////
// Card->Reader Direction Traffic
/////////////////////////////////////////////////

INLINE void Insert0(void)
{
    SampleRegister >>= 1;
    if (++BitCount % 8)
        return;
    *CodecBufferPtr++ = SampleRegister;
}

INLINE void Insert1(void)
{
    SampleRegister = (SampleRegister >> 1) | 0x80;
    if (++BitCount % 8)
        return;
    *CodecBufferPtr++ = SampleRegister;
}

// This interrupt find Card -> Reader SOC
ISR(ACA_AC0_vect) // this interrupt either finds the SOC or gets triggered before
{
//    LED_PORT.OUTSET=LED_RED;

    ACA.AC0CTRL &= ~AC_INTLVL_HI_gc; // disable this interrupt
    // enable the pause-finding timer
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH2_gc;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_DIV1_gc;

}
// Decode the Card -> Reader signal
// according to the pause and modulated period
// if the half bit duration is modulated, then add 1 to buffer
// if the half bit duration is not modulated, then add 0 to buffer
ISR(CODEC_TIMER_LOADMOD_CCB_VECT) // pause found
{
    uint8_t tmp = CODEC_TIMER_TIMESTAMPS.CNTL;
    CODEC_TIMER_TIMESTAMPS.CNT = 0;

    /* This needs to be done only on the first call,
     * but doing this only on a condition means wasting time, so we do it every time. */
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_DIV4_gc;

    switch (tmp) // decide how many half bit periods have been modulations
    {
        case 0 ... 48: // 32 ticks is one half of a bit period
            return;

        case 49 ... 80: // 64 ticks are a full bit period
            Insert1();
            Insert0();
            return;

        case 81 ... 112: // 96 ticks are 3 half bit periods
            if (BitCount & 1)
            {
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
}
// EOC of Card->Reader found
ISR(CODEC_TIMER_TIMESTAMPS_CCB_VECT) // EOC found
{
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = 0;
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_OFF_gc;
    ACA.AC0CTRL &= ~AC_ENABLE_bm;

    if (BitCount & 1)
    {
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


INLINE void CardSniffInit(void)
{
    TrafficSource = TRAFFIC_CARD;

    /* Initialize common peripherals and start listening
     * for incoming data. */


    // No need to implement FDT, timer disabled
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLA = 0;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCBINTLVL_OFF_gc;

    CODEC_TIMER_LOADMOD.CTRLA = 0;
    State = STATE_IDLE;

    CodecBufferPtr = CodecBuffer; // use GPIOR for faster access
    BitCount = 1; // FALSCH todo the first modulation of the SOC is "found" implicitly
    SampleRegister = 0x00;

    Flags.RxDone = false;
    Flags.Start = true;
    // reset for future use
//    CodecBufferIdx = 0;
//    BitCountUp = 0;

    Flags.RxPending = true;

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
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = TC_CCBINTLVL_LO_gc;

    /* Use the event system for resetting the pause-detecting timer. */
    EVSYS.CH2MUX = EVSYS_CHMUX_ACA_CH0_gc; // on every ACA_AC1 INT
    EVSYS.CH2CTRL = EVSYS_DIGFILT_1SAMPLE_gc;

    CODEC_DEMOD_IN_PORT.INTCTRL = 0;

    /* Enable the AC interrupt, which either finds the SOC and then starts the pause-finding timer,
     * or it is triggered before the SOC, which mostly isn't bad at all, since the first pause
     * needs to be found. */
    ACA.AC1CTRL = 0;
    ACA.STATUS = AC_AC0IF_bm;
    ACA.AC0CTRL = AC_HSMODE_bm | AC_HYSMODE_NO_gc | AC_INTMODE_FALLING_gc | AC_INTLVL_HI_gc | AC_ENABLE_bm;

//    RxPendingSince = SystemGetSysTick();
}

INLINE void CardSniffDeinit(void)
{
    //    CodecSetDemodPower(false);
    // CodecReaderFieldStop();
    CODEC_TIMER_SAMPLING.CTRLA = 0;
    CODEC_TIMER_SAMPLING.INTCTRLB = 0;
    CODEC_TIMER_LOADMOD.CTRLA = 0;
    CODEC_TIMER_LOADMOD.INTCTRLB = 0;



    EVSYS.CH2MUX = 0; // on every ACA_AC1 INT
    EVSYS.CH2CTRL = 0;
    ACA.AC0MUXCTRL = AC_MUXPOS_DAC_gc | AC_MUXNEG_PIN7_gc;
    ACA.AC0CTRL = CODEC_AC_DEMOD_SETTINGS;



    Flags.RxDone = false;
    Flags.RxPending = false;
    Flags.Start = false;

//    Reader14443ACodecDeInit();
}


/////////////////////////////////////////////////
// Init and deInit, task, functions for this codec
/////////////////////////////////////////////////

void Sniff14443ACodecInit(void)
{
    // Common Codec Register settings
    CodecInitCommon();
    // Enable demodulator power
    CodecSetDemodPower(true);

    // Start with sniffing Reader->Card direction traffic
    ReaderSniffInit();

}

void Sniff14443ACodecDeInit(void)
{
//    SniffEnable = false;
    CardSniffDeinit();
    ReaderSniffDeInit();
    CodecSetDemodPower(false);
}


void Sniff14443ACodecTask(void)
{

    if (TrafficSource == TRAFFIC_READER){
        if (Flags.DemodFinished) {
            Flags.DemodFinished = 0;
            /* Reception finished. Process the received bytes */
            uint16_t DemodBitCount = BitCount;

            if (DemodBitCount >= ISO14443A_MIN_BITS_PER_FRAME) {
                // For logging data
                LogEntry(LOG_INFO_CODEC_RX_DATA, CodecBuffer, (DemodBitCount+7)/8);
                LEDHook(LED_CODEC_RX, LED_PULSE);

                ReaderSniffDeInit();
                CardSniffInit();

                return;
            }
            // Get nothing, Start sniff again
            ReaderSniffInit();
        }


    }

    else    // Card->Reader Task
    {

        // Waiting for response time out
//    if (Flags.RxPending && SYSTICK_DIFF(RxPendingSince) > Reader_FWT + 1)
//    {
//        Reader14443A_EOC();
//        BitCount = 0;
//        Flags.RxDone = true;
//        Flags.RxPending = false;
//    }
//    if (CodecIsReaderToBeRestarted() || !CodecIsReaderFieldReady())
//        return;
        if (!Flags.RxPending && (Flags.Start || Flags.RxDone))
        {

//        if (State == STATE_FDT && CODEC_TIMER_LOADMOD.CNT < ISO14443A_PICC_TO_PCD_MIN_FDT) { // we are in frame delay time, so we can return later
//            return;
//        }

            if (Flags.RxDone && BitCount > 0) // decode the raw received data
            {

                if (BitCount < ISO14443A_RX_MINIMUM_BITCOUNT * 2)
                {
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
                    while (!breakflag && BitCountTmp < TotalBitCount)
                    {
                        uint8_t Bit = TmpCodecBuffer[BitCountTmp / 8] & 0x03;
                        TmpCodecBuffer[BitCountTmp / 8] >>= 2;
                        switch (Bit)
                        {
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

//                BitCount = removeParityBits(CodecBuffer, BitCount);

                    LogEntry(LOG_INFO_CODEC_RX_DATA_W_PARITY, CodecBuffer, (BitCount + 7) / 8);

                    Flags.RxPending=true;

                    // Disable card sniffing and enable reader sniffing
                    CardSniffDeinit();
                    ReaderSniffInit();

                    return;
                }

            }
            Flags.Start = false;
            Flags.RxDone = false;

            /* Call application with received data */
            BitCount = ApplicationProcess(CodecBuffer, BitCount);

        }
    }
}



