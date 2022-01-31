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
uint8_t CodecBuffer2[CODEC_BUFFER_SIZE];

enum RCTraffic SniffTrafficSource;

void (* volatile isr_func_TCD0_CCC_vect)(void) = NULL;
void (* volatile isr_func_CODEC_DEMOD_IN_INT0_VECT)(void) = NULL;
void (* volatile isr_func_ACA_AC0_vect)(void);
void (* volatile isr_func_CODEC_TIMER_LOADMOD_OVF_VECT)(void) = NULL;
void (* volatile isr_func_CODEC_TIMER_LOADMOD_CCA_VECT)(void) = NULL;
void (* volatile isr_func_CODEC_TIMER_LOADMOD_CCB_VECT)(void) = NULL;
void (* volatile isr_func_CODEC_TIMER_TIMESTAMPS_CCA_VECT)(void) = NULL;

// the following three functions prevent sending data directly after turning on the reader field
void CodecReaderFieldStart(void) { // DO NOT CALL THIS FUNCTION INSIDE APPLICATION!
    if (!CodecGetReaderField() && !ReaderFieldFlags.ToBeRestarted) {
        CodecSetReaderField(true);
        ReaderFieldFlags.Started = true;
        ReaderFieldFlags.Ready = false;
        ReaderFieldStartTimestamp = SystemGetSysTick();
    }
}

void CodecReaderFieldStop(void) {
    CodecSetReaderField(false);
    ReaderFieldFlags.Started = false;
    ReaderFieldFlags.Ready = false;
}

void CodecReaderFieldRestart(uint16_t delay) {
    ReaderFieldFlags.ToBeRestarted = true;
    ReaderFieldRestartTimestamp = SystemGetSysTick();
    ReaderFieldRestartDelay = delay;
    CodecReaderFieldStop();
}

/*
 * This function returns false if and only if the reader field has been turned on in the last READER_FIELD_MIN..._TIME ms.
 * If the reader field is not active, true is returned.
 */
bool CodecIsReaderFieldReady(void) {
    if (!ReaderFieldFlags.Started)
        return true;
    if (ReaderFieldFlags.Ready || (ReaderFieldFlags.Started && SYSTICK_DIFF(ReaderFieldStartTimestamp) >= READER_FIELD_MINIMUM_WAITING_TIME)) {
        ReaderFieldFlags.Ready = true;
        return true;
    }
    return false;
}

bool CodecIsReaderToBeRestarted(void) {
    if (ReaderFieldFlags.ToBeRestarted) {
        if (SYSTICK_DIFF(ReaderFieldRestartTimestamp) >= ReaderFieldRestartDelay) {
            ReaderFieldFlags.ToBeRestarted = false;
            CodecReaderFieldStart();
        }
        return true;
    }
    return false;
}

void CodecThresholdSet(uint16_t th) { // threshold has to be saved back to eeprom by the caller, if wanted
    GlobalSettings.ActiveSettingPtr->ReaderThreshold = th;
    DACB.CH0DATA = th;
}

uint16_t CodecThresholdIncrement(void) { // threshold has to be saved back to eeprom by the caller, if wanted
    GlobalSettings.ActiveSettingPtr->ReaderThreshold += CODEC_THRESHOLD_CALIBRATE_STEPS;
    DACB.CH0DATA = GlobalSettings.ActiveSettingPtr->ReaderThreshold;
    return GlobalSettings.ActiveSettingPtr->ReaderThreshold;
}

void CodecThresholdReset(void) { // threshold has to be saved back to eeprom by the caller, if wanted
    GlobalSettings.ActiveSettingPtr->ReaderThreshold = DEFAULT_READER_THRESHOLD;
    DACB.CH0DATA = GlobalSettings.ActiveSettingPtr->ReaderThreshold;
}

