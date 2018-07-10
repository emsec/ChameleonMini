//
// Created by Zitai Chen on 05/07/2018.
// Some interrupt handling vect
// modified form ISO14443-2A.c and Reader14443-2A.c
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
#define SAMPLE_RATE_SYSTEM_CYCLES		((uint16_t) (((uint64_t) F_CPU * ISO14443A_BIT_RATE_CYCLES) / CODEC_CARRIER_FREQ) )

#define ISO14443A_MIN_BITS_PER_FRAME		7


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

bool SniffEnable;
enum RCTraffic TrafficSource;

static volatile struct {
    volatile bool DemodFinished;
//    volatile bool LoadmodFinished;
} Flags = { 0 };

typedef enum {
    /* Demod */
            DEMOD_DATA_BIT,
    DEMOD_PARITY_BIT,
    LOADMOD_FDT,

} StateType;

/////////////////////////////////////////////////
// Reader->Card Direction Traffic
/////////////////////////////////////////////////

void ReaderSniffInit(void)
{
    /* Initialize some global vars and start looking out for reader commands */
    Flags.DemodFinished = 0;

//    ReaderStartDemod();

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
ISR(PORTB_INT1_vect) {
//    LED_PORT.OUTSET = LED_RED;
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

    /* Setup Frame Delay Timer and wire to EVSYS. Frame delay time is
     * measured from last change in RF field, therefore we use
     * the event channel 1 (end of modulation pause) as the restart event.
     * The preliminary frame delay time chosen here is irrelevant, because
     * the correct FDT gets set automatically after demodulation. */

//    CODEC_TIMER_LOADMOD.CNT = 0;
//    CODEC_TIMER_LOADMOD.PER = 0xFFFF;
////    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODEND_EVSEL;
////    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
////    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_OVFIF_bm;
////    CODEC_TIMER_LOADMOD.CTRLA = CODEC_TIMER_CARRIER_CLKSEL;
//
//    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
//    CODEC_TIMER_LOADMOD.INTCTRLA = 0;

    /* Disable this interrupt */
    CODEC_DEMOD_IN_PORT.INT1MASK = 0;
}

// Sampling with timer and demod
ISR(TCD0_CCD_vect) {
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

            /* By this time, the FDT timer is aligned to the last modulation
             * edge of the reader. So we disable the auto-synchronization and
             * let it count the frame delay time in the background, and generate
             * an interrupt once it has reached the FDT. */
            // Do not activate loadmod
//            CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;
//            CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
//            CODEC_TIMER_LOADMOD.INTCTRLA = 0;

//            if (SampleRegister & 0x08) {
//                CODEC_TIMER_LOADMOD.PER = ISO14443A_FRAME_DELAY_PREV1 - 40; /* compensate for ISR prolog */
//            } else {
//                CODEC_TIMER_LOADMOD.PER = ISO14443A_FRAME_DELAY_PREV0 - 40; /* compensate for ISR prolog */
//            }

            StateRegister = LOADMOD_FDT;

//            CODEC_TIMER_LOADMOD.INTFLAGS = TC0_OVFIF_bm;
//            CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_HI_gc;

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


//    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
//    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc;
//    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
//    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_OVFIF_bm;

}





/////////////////////////////////////////////////
// Card->Reader Direction Traffic
/////////////////////////////////////////////////

INLINE void CardSniffInit(void)
{
    TestSniff14443ACodecInit();
}

INLINE void CardSniffDeinit(void)
{
    Reader14443ACodecDeInit();
}



void Sniff14443ACodecInit(void)
{
    Flags.DemodFinished = 0;

    SniffEnable = true;
    TrafficSource = TRAFFIC_READER;

    // Common Codec Register settings
    CodecInitCommon();
    // Enable demodulator power
    CodecSetDemodPower(true);

    // Start with sniffing Reader->Card direction traffic
    // Card -> Reader traffic sniffing interrupt will be enbaled
    // when Reader-> Card direction sniffing finished
    // See ISO14443-2A.c ISO14443ACodecTask()
    ReaderSniffInit();

    // Also configure Interrupt settings for Reader function
//    Reader14443ACodecInit();
}

void Sniff14443ACodecDeInit(void)
{
    SniffEnable = false;
    CardSniffDeinit();
    ReaderSniffDeInit();
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

                if(SniffEnable == true){
                    // TODO: Start interrupt for finding Card -> Reader direction traffic

                    ReaderSniffDeInit();
                    CardSniffInit();

//                    ISO14443ACodecDeInit();

//                    TestSniff14443ACodecInit();
                    TrafficSource = TRAFFIC_CARD;
                    return;
                }
            }
//
//                /* No data to be processed. Disable loadmodding and start listening again */
//                CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
//                CODEC_TIMER_LOADMOD.INTCTRLA = 0;

                ReaderSniffInit();

        }



    } else{
        Reader14443ACodecTask();
    }
//    ISO14443ACodecDeInit();
//    Reader14443ACodecTask();
}



