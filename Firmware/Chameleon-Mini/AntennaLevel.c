#include "AntennaLevel.h"
#include "LEDHook.h"

#define FIELD_MIN_RSSI 500

void AntennaLevelTick(void)
{
	uint16_t rssi = AntennaLevelGet();

	if (rssi < FIELD_MIN_RSSI)
		LEDHook(LED_FIELD_DETECTED, LED_OFF);
	else
		LEDHook(LED_FIELD_DETECTED, LED_ON);
}
