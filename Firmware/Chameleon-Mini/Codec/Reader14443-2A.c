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
	STATE_MILLER_SOF0,
	STATE_MILLER_SOF1,
	STATE_MILLER_TX0,
	STATE_MILLER_TX1,
	STATE_MILLER_EOF0,
	STATE_MILLER_EOF1,
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
	CodecSetDemodPower(true);

    CODEC_TIMER_SAMPLING.PER = SAMPLE_RATE_SYSTEM_CYCLES/2 - 1;
    CODEC_TIMER_SAMPLING.CCB = 1;
    CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_SAMPLING.INTCTRLA = 0;
    CODEC_TIMER_SAMPLING.INTCTRLB = TC_CCBINTLVL_HI_gc;

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

INLINE void Reader14443A_EOC(void)
{
	CODEC_TIMER_LOADMOD.INTCTRLB = 0;
    CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_OFF_gc;
    CODEC_TIMER_TIMESTAMPS.INTCTRLB = 0;
    CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_OFF_gc;
    ACA.AC1CTRL &= ~AC_ENABLE_bm;

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

ISR(CODEC_TIMER_SAMPLING_CCB_VECT)
{
	static void * JumpTable[] = {
			[STATE_IDLE] = &&STATE_IDLE_LABEL,
			[STATE_MILLER_SOF0] = &&STATE_MILLER_SOF0_LABEL,
			[STATE_MILLER_SOF1] = &&STATE_MILLER_SOF1_LABEL,
			[STATE_MILLER_TX0] = &&STATE_MILLER_TX0_LABEL,
			[STATE_MILLER_TX1] = &&STATE_MILLER_TX1_LABEL,
			[STATE_MILLER_EOF0] = &&STATE_MILLER_EOF0_LABEL,
			[STATE_MILLER_EOF1] = &&STATE_MILLER_EOF1_LABEL,
			[STATE_FDT] = &&STATE_FDT_LABEL
	};

	goto *JumpTable[State];

	STATE_MILLER_SOF0_LABEL:
		/* Output a SOF */
		CodecSetReaderField(false);
		_delay_us(2.5);
		CodecSetReaderField(true);

		CodecBufferIdx = 0;
		SampleRegister = CodecBuffer[0];
		BitCountUp = 0;

		State = STATE_MILLER_SOF1;
		return;

	STATE_MILLER_SOF1_LABEL:
		State = STATE_MILLER_TX0;
		return;

	STATE_MILLER_TX0_LABEL:
		/* First half of bit-slot */
		if (SampleRegister & 0x01) {
			/* Do Nothing */
		} else {
			if (LastBit == 0) {
				CodecSetReaderField(false);
				_delay_us(2.5);
				CodecSetReaderField(true);
			} else {
				/* Do Nothing */
			}
		}

		State = STATE_MILLER_TX1;
		return;

	STATE_MILLER_TX1_LABEL:
		/* Second half of bit-slot */
		if (SampleRegister & 0x01) {
			CodecSetReaderField(false);
			_delay_us(2.5);
			CodecSetReaderField(true);
			LastBit = 1;
		} else {
			/* No modulation pauses */
			LastBit = 0;
		}

		SampleRegister >>= 1;
		if (--BitCount == 0) {
			State = STATE_MILLER_EOF0;
		} else {
			State = STATE_MILLER_TX0;
		}

		if ((++BitCountUp % 8) == 0)
			SampleRegister = CodecBuffer[++CodecBufferIdx];
		return;

	STATE_MILLER_EOF0_LABEL:
		if (LastBit == 0) {
			CodecSetReaderField(false);
			_delay_us(2.5);
			CodecSetReaderField(true);
		} else {
			/* Do Nothing */
		}

		State = STATE_MILLER_EOF1;
		return;

		STATE_MILLER_EOF1_LABEL:
			CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_OFF_gc;
	        _delay_us(50); // wait for DEMOD signal to stabilize

	        /* Enable the AC interrupt, which either finds the SOC and then starts the pause-finding timer,
	         * or it is triggered before the SOC, which mostly isn't bad at all, since the first pause
	         * needs to be found. */
			ACA.STATUS = AC_AC1IF_bm;
	        ACA.AC1CTRL = AC_HSMODE_bm | AC_HYSMODE_NO_gc | AC_INTMODE_BOTHEDGES_gc | AC_INTLVL_HI_gc | AC_ENABLE_bm;

	        CodecBufferPtr = CodecBuffer; // use GPIOR for faster access
	        BitCount = 1; // the first modulation of the SOC is "found" implicitly
	        SampleRegister = 0x80;

	        RxPendingSince = SystemGetSysTick();
	        Flags.RxPending = true;

	        // reset for future use
	        CodecBufferIdx = 0;
	        BitCountUp = 0;

			State = STATE_IDLE;
			return;

		STATE_IDLE_LABEL:
		STATE_FDT_LABEL:
			return;
	}

ISR(CODEC_TIMER_TIMESTAMPS_CCA_VECT) // EOC found
{
	Reader14443A_EOC();
}

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

ISR(ACA_AC1_vect) // this interrupt either finds the SOC or gets triggered before
{
	ACA.AC1CTRL &= ~AC_INTLVL_HI_gc; // disable this interrupt
	// enable the pause-finding timer
	CODEC_TIMER_LOADMOD.CTRLD = TC_EVACT_RESTART_gc | TC_EVSEL_CH0_gc;
	CODEC_TIMER_LOADMOD.CTRLA = TC_CLKSEL_DIV1_gc;
}

ISR(CODEC_TIMER_LOADMOD_CCA_VECT) // pause found
{
	uint8_t tmp = CODEC_TIMER_TIMESTAMPS.CNTL;
	CODEC_TIMER_TIMESTAMPS.CNT = 0;

	/* This needs to be done only on the first call,
	 * but doing this only on a condition means wasting time, so we do it every time. */
	CODEC_TIMER_TIMESTAMPS.CTRLA = TC_CLKSEL_DIV2_gc;

	switch (tmp) // decide how many half bit periods have been modulations
	{
	case 0 ... 96: // 64 ticks is one half of a bit period
		Insert0();
		// If one full bit period is over AND the last half bit period also was a pause,
		// we have found the EOC.
		if ((BitCount & 1) == 0 && (SampleRegister & 0x40) == 0)
			Reader14443A_EOC();
		return;

	case 97 ... 160: // 128 ticks is a full bit period
		Insert1();
		Insert0();
		return;

	default: // every value over 128 + 32 (tolerance) is considered to be 3 half bit periods
		Insert1();
		Insert1();
		Insert0();
		return;
	}
	return;
}

void Reader14443ACodecTask(void)
{
	if (Flags.RxPending && SYSTICK_DIFF(RxPendingSince) > Reader_FWT + 1)
	{
		BitCount = 0;
		Flags.RxDone = true;
		Flags.RxPending = false;
	}
	if (CodecIsReaderToBeRestarted() || !CodecIsReaderFieldReady())
		return;
	if (!Flags.RxPending && (Flags.Start || Flags.RxDone))
	{
		if (State == STATE_FDT && CODEC_TIMER_LOADMOD.CNT < ISO14443A_PICC_TO_PCD_MIN_FDT) // we are in frame delay time, so we can return later
			return;
		if (Flags.RxDone && BitCount > 0) // decode the raw received data
		{
			if (BitCount < ISO14443A_RX_MINIMUM_BITCOUNT * 2)
			{
				BitCount = 0;
			} else {
				uint8_t TmpCodecBuffer[CODEC_BUFFER_SIZE];
				memcpy(TmpCodecBuffer, CodecBuffer, (BitCount + 7) / 8);

				CodecBufferPtr = CodecBuffer;
				uint16_t BitCountTmp = 0, TotalBitCount = BitCount;
				BitCount = 0;

				bool breakflag = false;

				while (!breakflag && BitCountTmp < TotalBitCount)
				{
					uint8_t Bit = TmpCodecBuffer[BitCountTmp / 8] & 0x03;
					TmpCodecBuffer[BitCountTmp / 8] >>= 2;
					switch (Bit)
					{
					case 0b10:
						Insert0();
						break;

					case 0b01:
						Insert1();
						break;

					case 0b00: // EOC
						if (BitCount % 8) // copy the last byte, if there is an incomplete byte
							CodecBuffer[BitCount / 8] = SampleRegister >> (8 - (BitCount % 8));
						LEDHook(LED_CODEC_RX, LED_PULSE);
						LogEntry(LOG_INFO_CODEC_RX_DATA_W_PARITY, CodecBuffer, (BitCount + 7) / 8);
						breakflag = true;
						break;

					default:
						// error, should not happen, TODO handle this
						break;
					}
					BitCountTmp += 2;
				}
			}
		}
		Flags.Start = false;
		Flags.RxDone = false;

		/* Call application with received data */
		BitCount = ApplicationProcess(CodecBuffer, BitCount);

		if (BitCount > 0)
		{
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
			ACA.AC1CTRL = AC_HSMODE_bm | AC_HYSMODE_NO_gc | AC_INTMODE_BOTHEDGES_gc | AC_ENABLE_bm;

			/* This timer will be used to detect the pauses between the modulation sequences. */
			CODEC_TIMER_LOADMOD.CTRLA = 0;
			CODEC_TIMER_LOADMOD.CNT = 0;
			CODEC_TIMER_LOADMOD.PER = 127; // with 27.12 MHz this is exactly one half bit width
			CODEC_TIMER_LOADMOD.CCA = 95; // with 27.12 MHz this is 3/4 of a half bit width
			CODEC_TIMER_LOADMOD.INTCTRLA = 0;
			CODEC_TIMER_LOADMOD.INTFLAGS = TC1_CCAIF_bm;
			CODEC_TIMER_LOADMOD.INTCTRLB = TC_CCAINTLVL_HI_gc;

			/* This timer will be used to find out how many bit halfs since the last pause have been passed. */
			CODEC_TIMER_TIMESTAMPS.CNT = 0;
			CODEC_TIMER_TIMESTAMPS.PER = 0xFFFF;
			CODEC_TIMER_TIMESTAMPS.CCA = 256;
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
			State = STATE_MILLER_SOF0;
			LastBit = 0;
			CODEC_TIMER_SAMPLING.CNT = 0;
			CODEC_TIMER_SAMPLING.CTRLA = TC_CLKSEL_DIV1_gc;
		}
	}
}

void Reader14443ACodecStart(void)
{
	/* Application wants us to start a card transaction */
	BitCount = 0;
	Flags.Start = true;

	CodecReaderFieldStart();
}

void Reader14443ACodecReset(void)
{
	Reader14443A_EOC(); // this breaks every interrupt etc.
	State = STATE_IDLE;
	Flags.RxDone = false;
	Flags.Start = false;
	CodecReaderFieldStop();
}
