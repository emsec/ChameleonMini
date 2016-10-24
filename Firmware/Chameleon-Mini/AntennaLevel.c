#include "AntennaLevel.h"
#include "LEDHook.h"
#include "Application/Application.h"

#define FIELD_MIN_RSSI 500

void AntennaLevelTick(void)
{
	uint16_t rssi = AntennaLevelGet();

	if (rssi < FIELD_MIN_RSSI)
	{
		LEDHook(LED_FIELD_DETECTED, LED_OFF);
		if (ActiveConfiguration.UidSize != 0) // this implies that we are emulating right now
			ApplicationReset(); // reset the application just like a real card gets reset when there is no field
	} else {
		LEDHook(LED_FIELD_DETECTED, LED_ON);
	}
}
