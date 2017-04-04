/*
 * CODEC.c
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#include "Codec.h"
#include "../System.h"
#include "../LEDHook.h"

uint16_t Reader_FWT = ISO14443A_RX_PENDING_TIMEOUT;

#define READER_FIELD_MINIMUM_WAITING_TIME	70 // ms

static uint16_t ReaderFieldStartTimestamp = 0;
static uint16_t ReaderFieldRestartTimestamp = 0;
static uint16_t ReaderFieldRestartDelay = 0;
static volatile struct {
	bool Started;
	bool Ready;
	bool ToBeRestarted;
} ReaderFieldFlags = { false };

uint8_t CodecBuffer[CODEC_BUFFER_SIZE];
uint16_t ReaderThreshold = 400; // standard value

// the following three functions prevent sending data directly after turning on the reader field
void CodecReaderFieldStart(void) // DO NOT CALL THIS FUNCTION INSIDE APPLICATION!
{
	if (!CodecGetReaderField() && !ReaderFieldFlags.ToBeRestarted)
	{
		CodecSetReaderField(true);
		ReaderFieldFlags.Started = true;
		ReaderFieldFlags.Ready = false;
		ReaderFieldStartTimestamp = SystemGetSysTick();
	}
}

void CodecReaderFieldStop(void)
{
	CodecSetReaderField(false);
	ReaderFieldFlags.Started = false;
	ReaderFieldFlags.Ready = false;
}

void CodecReaderFieldRestart(uint16_t delay)
{
	ReaderFieldFlags.ToBeRestarted = true;
	ReaderFieldRestartTimestamp = SystemGetSysTick();
	ReaderFieldRestartDelay = delay;
	CodecReaderFieldStop();
}

/*
 * This function returns false if and only if the reader field has been turned on in the last READER_FIELD_MIN..._TIME ms.
 * If the reader field is not active, true is returned.
 */
bool CodecIsReaderFieldReady(void)
{
	if (!ReaderFieldFlags.Started)
		return true;
	if (ReaderFieldFlags.Ready || (ReaderFieldFlags.Started && SYSTICK_DIFF(ReaderFieldStartTimestamp) >= READER_FIELD_MINIMUM_WAITING_TIME))
	{
		ReaderFieldFlags.Ready = true;
		return true;
	}
	return false;
}

bool CodecIsReaderToBeRestarted(void)
{
	if (ReaderFieldFlags.ToBeRestarted)
	{
		if (SYSTICK_DIFF(ReaderFieldRestartTimestamp) >= ReaderFieldRestartDelay)
		{
			ReaderFieldFlags.ToBeRestarted = false;
			CodecReaderFieldStart();
		}
		return true;
	}
	return false;
}

