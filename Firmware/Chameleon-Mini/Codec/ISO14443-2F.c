/*
 * ISO14443-2F.c
 *
 * This code implements LEGIC prime physical layer for card emulation.
 *
 *  Created on: 5.7.2024
 *      Author: Ladislav Marko
 *  Inspired by ISO14443-2A.c and ISO15693.c
 */

#include "ISO14443-2F.h"
#include "../System.h"
#include "../Application/Application.h"
#include "../LEDHook.h"
#include "Codec.h"
#include "Log.h"

// ------------------------ LEGIC CODEC SECTION -------------------------------
// For reading the reader's data:
// Sampling of the reader is done using internal clock, synchronized to the first field modulation pause.
//
// Reader's "bitrate" is variable. Received bit 1 takes 80us HIGH and a bit 0 takes 40us HIGH.
// After both, there needs to be 20us of LOW.
//
// We can thus say, that 1 is composited of 80us of HIGH and 20us LOW, which takes 100us in total,
// and 0 is composited of 40us of HIGH and 20us of LOW, which takes 60us in total.
// Greatest common divisor of 100 and 60 is 20, so we need to measure every 20us to be sure we are synced.
// Thus, every time we measure LOW, we'll look into our memory and if we encountered
// 4 HIGHs before, we have just received a 1 or if we read just 2 HIGHs, we have received a 0.
// Effectively we need to sample each 20us which makes our bitrate 50 kbps
// which in turn makes our BIT_RATE_CYCLES 542 (.4)
//
// v ^
// o |
// l |          1                 0                   1                     0
// t |  +----------------+    +--------+    +--*----*----*----*--+  L  +--*----*--+  L
// a |  |                |    |        |    |  *    *    *    *  |  *  |  *    *  |  *
// g |  |                |    |        |    |  *    *    *    *  |  *  |  *    *  |  *
// e |  |                +----+        +----+  H    H    H    H  +--*--+  H    H  +--*--
//   +-------------------------------------------------------------------------------------------> time
//
// NOTE: a dash takes 5us, stars symbolise a measurement that should be every 20us
// we decode HHHHL as 1 and HHL as 0
//
//
// For sending data:
// Sending uses the carrier frequency as a clock source and is synchronized to reader's last modulation pause.
//
// We need to wait +-320us after last reader bit and then start sending card data.
// We use on-off keying with 1/64 of the carrier wave as the subcarrier with bit duration of 100us.
//
// The reader usually responds after 230us with new data.

// This is +-20 microseconds
#define READER_SIGNAL_SAMPLE_RATE_IN_SYSTEM_CYCLES		((uint16_t) (((uint64_t) F_CPU * ISO14443F_BIT_RATE_CYCLES) / CODEC_CARRIER_FREQ))

// This is +-30 microseconds
#define FIRST_SAMPLING_OFFSET_IN_SYSTEM_CYCLES 659

//This is +-100 microseconds
#define TRANSMIT_RATE_IN_SYSTEM_CYCLES  1361

//This is +-320 microseconds
#define FIRST_TRANSMIT_OFFSET_IN_SYSTEM_CYCLES 4334

// Enum for states of the recieve functions that demodulate data from the reader
// The cycle should be DONT -> DO -> END -> DONT
typedef enum {
    DONT_RECEIVE,
    DO_RECEIVE,
    END_RECEIVE //Give away control after all data have been received
} ReceiveStateType;

// Enum for states of the transmit functions that modulate data to the reader
// The cycle should be NONE -> START -> BIT -> BIT -> ... -> BIT -> END -> NONE
typedef enum {
    TRANSMIT_NONE,
    TRANSMIT_START,
    TRANSMIT_BIT,
    TRANSMIT_END
} TransmitStateType;

/* Define pseudo variables to use fast register access. This is useful for global vars */
#define ReceiveStateRegister	Codec8Reg0
#define TransmitStateRegister	Codec8Reg1
#define SampleIdxRegister Codec8Reg2
#define SampleRegister	Codec8Reg3
#define BitSent			CodecCount16Register1
#define BitCount		CodecCount16Register2

/* Set pin PE0 to HIGH - used only for debugging during development */
INLINE void set_PE0_high(void){
    PORTE.DIRSET |= PIN0_bm;
    PORTE.OUTSET |= PIN0_bm;
}

/* Set pin PE0 to LOW - used only for debugging during development */
INLINE void set_PE0_low(void){
    PORTE.OUTCLR |= PIN0_bm;
}

/* Handles the end of reading data from the reader when the physical layer data make sense */
INLINE void ISO14443_F_DEMOD_END(void) {
    SampleIdxRegister = 0;
    /* Disable demodulation interrupt */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc; /* Disconnect system clock from demod timer */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc; /* Remove action from timer */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCAINTLVL_OFF_gc; /* Disable CCA interrupts */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_CCAIF_bm; /* Clear CCA interrupt flag */

    /* By this time, the LOADMOD timer is aligned to the last modulation
     * edge of the reader. So we disable the auto-synchronization and
     * let it count the frame delay time in the background, and generate
     * an interrupt once it has reached the FDT. */
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_OFF_gc; /* Disable restarts on modulation ends */
    CODEC_TIMER_LOADMOD.PERBUF = TRANSMIT_RATE_IN_SYSTEM_CYCLES; /* Prepare to change state every 100 us */
    CODEC_TIMER_LOADMOD.PER = FIRST_TRANSMIT_OFFSET_IN_SYSTEM_CYCLES; /* +- 320 microseconds offset from now */
    CODEC_TIMER_LOADMOD.INTFLAGS = TC0_OVFIF_bm; /* Clear overflow interrupt flag */
    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_HI_gc; /* Set overflow interrupt level to high */

    TransmitStateRegister = TRANSMIT_START;
    ReceiveStateRegister = END_RECEIVE;

}

// v ^                                          Trigger here
// o |                                                |
// l |  Reader charging card/previous communication   V            1                 0
// t |------------------------------------------------|    +----------------+    +--------+
// a |                                                |    |                |    |        |
// g |                                                |    |                |    |        |
// e |                                                |----|                +----+        +-----
//   +-------------------------------------------------------------------------------------------> time
void EnableFirstModulationPauseInterrupt(void){
    /* Start looking out for modulation pause via interrupt. */
    CODEC_DEMOD_IN_PORT.INTFLAGS = PORT_INT0IF_bm; /* Clear the Interrupt 0 flag on the DEMOD port (Port B) */
    /* Set Pin 1 as source for Interrupt 0 on the DEMOD port (Port B) */
    CODEC_DEMOD_IN_PORT.INT0MASK = CODEC_DEMOD_IN_MASK0;
}

/* Handles the end of reading data from the reader when the physical layer data don't make sense */
INLINE void ISO14443_F_GARBAGE(void){
    SampleIdxRegister = 0;
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc; /* Disconnect system clock from demod timer */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc; /* Remove action from timer */
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_OVFINTLVL_OFF_gc; /* Disable CCA interrupts */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_OVFIF_bm; /* Clear OVF interrupt flag */
    EnableFirstModulationPauseInterrupt(); /* Start listening for the reader's field changes again */
}

/* Starts Loadmod timer as a free-running timer and syncs it to reader's modulation ends, so it will be accurate when
 * we really need to start using it later */
void PrepareLoadmodTimer(void){
    CODEC_TIMER_LOADMOD.CTRLA = CODEC_TIMER_CARRIER_CLKSEL; /* Use Carrier wave as timer source */
    CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | CODEC_TIMER_MODEND_EVSEL; /* Restart time on modulation ends */
    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc; /* Disable interrupt on overflow */
    CODEC_TIMER_LOADMOD.CNT = 0;
    CODEC_TIMER_LOADMOD.PER = 0xFFFF; /* Set period to too high of a value */
}

/* Starts waiting for first demodulation pause to start sampling reader's field (and also prepares Loadmod timer) */
INLINE void StartDemod(void) {
    PrepareLoadmodTimer();

    /* Activate Power for demodulator */
    CodecSetDemodPower(true);
    ReceiveStateRegister = DO_RECEIVE;
    SampleRegister = 0;
    SampleIdxRegister = 0;
    BitCount = 0;

    EnableFirstModulationPauseInterrupt();
}

/* This handles the interrupt enabled in EnableFirstModulationPauseInterrupt()
 *
 * This should trigger at the start of the reader's first modulation pause. It then starts the sampling timer
 * to simply wait for a predefined interval, to align the timer about 10us into the first data transmission from
 * the reader, and then to start sampling each 20 microseconds with the counter overflow (OVF) event
 * v ^    NOW       PER2 PER2 PER2
 * o |     |--PER--|    |    |    |
 * l |     V       |    |    |    |
 * t | ----+    +--*----*----*----*--+  O  +--*----*--+  O
 * a |     |    |  *    *    *    *  |  *  |  *    *  |  *
 * g |     |    |  *    *    *    *  |  *  |  *    *  |  *
 * e |     +----+  O    O    O    O  +--*--+  O    O  +--*--
 *   +-------------------------------------------------------------------------------------------> time
 *
 * a dash takes 5us, O symbolises CODEC_TIMER_SAMPLING overflow interrupt that will get handled by
 * the isr_ISO14443_2F_CODEC_TIMER_SAMPLING_OVF_VECT function

 * NOTE: PERBUF will become PER the first time CNT==PER will be true, here denoted PER2
 * read chapter 14 about Timer/Counter Type 0 and 1 of ATxmega manual for more information */
ISR_SHARED isr_ISO14443_2F_CODEC_DEMOD_IN_INT0_VECT(void) {
    /* Configure sampling-timer free running and sync to first modulation-pause. */
    /* CodecInitCommon(); sets Event channel 0 to signal the beginning (rising edge) of a modulation pause and Event channel 1 to
     * signal the end (falling edge) of a modulation pause. */
    CODEC_TIMER_SAMPLING.CNT = 0; /* Reset the timer's initial value*/
    CODEC_TIMER_SAMPLING.PER = FIRST_SAMPLING_OFFSET_IN_SYSTEM_CYCLES; /* Set the timer's period, so we land +-10us into readers data signal */
    CODEC_TIMER_SAMPLING.PERBUF = READER_SIGNAL_SAMPLE_RATE_IN_SYSTEM_CYCLES; /* Set the timer's next period, so we sample each 20us */
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV1_gc;  /* Select the system clock (with no prescaler) as the timer source */
    CODEC_TIMER_SAMPLING.CTRLD = TC_EVACT_OFF_gc; /* Turn of any event actions */
    CODEC_TIMER_SAMPLING.INTCTRLA = TC_OVFINTLVL_HI_gc; /* Mark timer overflow interrupt as high level */
    CODEC_TIMER_SAMPLING.INTFLAGS = TC0_OVFIF_bm; /* Clear timer overflow interrupt flag */

    /* Disable this interrupt. From now on we will sample the field using our CODEC_TIMER_SAMPLING OVF interrupt */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;

}
INLINE void DisableLoadmodTimer(void){
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_LOADMOD.INTCTRLA = TC_OVFINTLVL_OFF_gc;
}

INLINE void SetBitOnPositionInBufferToValue(volatile uint8_t * buffer, uint16_t position, uint8_t value){
    uint16_t byte_offset = position / 8;
    uint8_t bit_offset = position % 8;

    if (value){
        buffer[byte_offset] |= (1 << bit_offset);        // Set bit to 1
    } else {
        buffer[byte_offset] &= ~(1 << bit_offset);       // Set bit to 0
    }
}

uint8_t GetBitOnPositionInBuffer(const uint8_t * buffer, uint16_t position){
    uint16_t byte_offset = position / 8;
    uint8_t bit_offset = position % 8;

    return (buffer[byte_offset] & (1 << bit_offset)) >> bit_offset;
}


// This triggers every 20us and samples the readers field
// from SamplePin's raw value which it converts to logical bits
// if 2 highs and a low are observed, it stores a logical 0 into CodecBuffer
// if 4 highs and a low are observed, it stores a logical 1 into CodecBuffer
ISR_SHARED isr_ISO14443_2F_CODEC_TIMER_SAMPLING_OVF_VECT(void){
    if (ReceiveStateRegister != DO_RECEIVE) {
        return;
    }
    SampleIdxRegister++;

    uint8_t SamplePin = CODEC_DEMOD_IN_PORT.IN & CODEC_DEMOD_IN_MASK;

    /* Shift sampled bit into sampling register */
    SampleRegister = (SampleRegister << 1) | (!SamplePin ? 0x01 : 0x00);

    if (!(SampleRegister & 0x1)) { // if last read bit is a zero
        //SynchronizeSamplingTimerToDemodEnd();
        if (!(SampleRegister ^ 0x1E)) {
            // We have read a 1
            SetBitOnPositionInBufferToValue(CodecBuffer, BitCount, 1);
            BitCount++;
        } else if (!(SampleRegister ^ 0x06)) {
            // We have read a 0
            SetBitOnPositionInBufferToValue(CodecBuffer, BitCount, 0);
            BitCount++;
        } else {
            ISO14443_F_GARBAGE();
        }
        SampleRegister = 0;
        SampleIdxRegister = 0;
    }

    if (SampleIdxRegister > 4) {
        // No more bits to be read or an error occurred during transmission as 5x HIGH should not happen
        if (BitCount > 0){
            ISO14443_F_DEMOD_END();
        } else {
            ISO14443_F_GARBAGE();
        }
    }
}

// Modulate as a card to send card response every 100 microseconds
ISR_SHARED isr_ISO14443_2F_CODEC_TIMER_LOADMOD_OVF_VECT(void) {
    static void *JumpTable[] = {
            [TRANSMIT_NONE] = && TRANSMIT_NONE_LABEL,
            [TRANSMIT_START] = && TRANSMIT_START_LABEL,
            [TRANSMIT_BIT] = && TRANSMIT_BIT_LABEL,
            [TRANSMIT_END] = && TRANSMIT_END_LABEL
    };

    if ((TransmitStateRegister >= TRANSMIT_NONE) && (TransmitStateRegister <= TRANSMIT_END)) {
        goto *JumpTable[TransmitStateRegister];
    } else {
        TerminalSendString("ERROR: Jump to unregistered label!\r\n");
        //TODO: Log error to memory as well
        return;
    }

TRANSMIT_NONE_LABEL:
    return;

TRANSMIT_START_LABEL:
    TransmitStateRegister = TRANSMIT_BIT;
    BitSent = 0;
    CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OOK, ISO14443F_SUBCARRIER_DIVIDER);
    CodecStartSubcarrier();
    /* Fallthrough */
TRANSMIT_BIT_LABEL:
    CodecSetLoadmodState(GetBitOnPositionInBuffer(CodecBuffer, BitSent));
    BitSent++;
    if (BitSent >= BitCount){
        TransmitStateRegister = TRANSMIT_END;
    }
    return;

TRANSMIT_END_LABEL:
    TransmitStateRegister = TRANSMIT_NONE;
    CodecSetLoadmodState(false);
    CodecSetSubcarrier(CODEC_SUBCARRIERMOD_OFF, 0);

    DisableLoadmodTimer();
    StartDemod();
}

void ISO14443FCodecInit(void) {
    /* Initialize some global vars and start looking out for reader commands */
    ReceiveStateRegister = DONT_RECEIVE;
    TransmitStateRegister = TRANSMIT_NONE;

    isr_func_CODEC_DEMOD_IN_INT0_VECT = &isr_ISO14443_2F_CODEC_DEMOD_IN_INT0_VECT;
    isr_func_CODEC_TIMER_SAMPLING_OVF_vect = &isr_ISO14443_2F_CODEC_TIMER_SAMPLING_OVF_VECT;
    isr_func_CODEC_TIMER_LOADMOD_OVF_VECT = &isr_ISO14443_2F_CODEC_TIMER_LOADMOD_OVF_VECT;
    CodecInitCommon();
    StartDemod();
}

void ISO14443FCodecDeInit(void) {
    /* Gracefully shutdown codec */
    CODEC_DEMOD_IN_PORT.INT0MASK = 0;
    ReceiveStateRegister = DONT_RECEIVE;
    TransmitStateRegister = TRANSMIT_NONE;


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

void ISO14443FCodecTask(void) {
    if (ReceiveStateRegister == END_RECEIVE) {
        LEDHook(LED_CODEC_RX, LED_PULSE); /* Signal data received */

        /* Zero out unused bytes for logging */
        for (uint16_t i = 0; i < (BitCount % 8); i++){
            CodecBuffer[(BitCount + 7) / 8] &= ~(1u << i);
        }
        LogEntry(LOG_INFO_CODEC_RX_DATA, CodecBuffer, (BitCount+7)/8 );


        uint16_t AnswerBitCount;
        AnswerBitCount = ApplicationProcess(CodecBuffer, BitCount);

        if (AnswerBitCount != ISO14443F_APP_NO_RESPONSE) {
            ReceiveStateRegister = DONT_RECEIVE;
            LogEntry(LOG_INFO_CODEC_TX_DATA, CodecBuffer, (AnswerBitCount + 7) / 8);
            BitCount = AnswerBitCount;
            TransmitStateRegister = TRANSMIT_START;
        } else {
            /* No data to be processed. Disable loadmodding and start listening again */
            DisableLoadmodTimer();
            StartDemod();
        }
    }
}