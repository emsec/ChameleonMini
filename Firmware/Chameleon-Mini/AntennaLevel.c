#include "AntennaLevel.h"
#include "LEDHook.h"
#include "Log.h"
#include "Application/Application.h"

uint8_t AntennaLevelLogReaderDetectCount = 0;

void AntennaLevelTick(void) {
    uint16_t rssi = AntennaLevelGet();

    if (rssi < FIELD_MIN_RSSI) {
        LEDHook(LED_FIELD_DETECTED, LED_OFF);
        if (ActiveConfiguration.UidSize != 0) // this implies that we are emulating right now
            ApplicationReset(); // reset the application just like a real card gets reset when there is no field
    } else {
        LEDHook(LED_FIELD_DETECTED, LED_ON);
        AntennaLevelLogReaderDetectCount = (++AntennaLevelLogReaderDetectCount) % ANTENNA_LEVEL_LOG_RDRDETECT_INTERVAL;
        if (AntennaLevelLogReaderDetectCount == 0) {
            uint8_t antLevel[2];
            antLevel[0] = (uint8_t)((rssi >> 8) & 0x00ff);
            antLevel[1] = (uint8_t)(rssi & 0x00ff);
            LogEntry(LOG_INFO_CODEC_READER_FIELD_DETECTED, antLevel, 2);
        }
    }
}
