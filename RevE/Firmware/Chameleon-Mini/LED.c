#include "LED.h"
#include "Settings.h"

#define BLINK_PRESCALER	1 /* x LEDTick(); */

LEDActionEnum LEDGreenAction = LED_NO_ACTION;
LEDActionEnum LEDRedAction = LED_NO_ACTION;

static const char PROGMEM LEDFuncTable[][32] =
{
    [LED_NO_FUNC] = "NONE",
    [LED_TERMINAL_CONN] = "TERMINAL_CONN",
    [LED_TERMINAL_RXTX] = "TERMINAL_RXTX",
    [LED_SETTING_CHANGE] = "SETTING_CHANGE",
    [LED_MEMORY_STORED] = "MEMORY_STORED",
    [LED_MEMORY_CHANGED] = "MEMORY_CHANGED",
    //TODO: [LED_FIELD_DETECTED] = "FIELD_DETECTED",
    [LED_CODEC_RX] = "CODEC_RX",
    [LED_CODEC_TX] = "CODEC_TX",
    //TODO: [LED_APP_SELECTED] = "APP_SELECTED",

};

INLINE void Tick(uint8_t Mask, LEDActionEnum* Action)
{
	static uint8_t BlinkPrescaler = 0;

	switch (*Action)
	{
		case LED_NO_ACTION:
			/* Do nothing */
			break;

		case LED_OFF:
			LED_PORT.OUTCLR = Mask;
			*Action = LED_NO_ACTION;
			break;

		case LED_ON:
			LED_PORT.OUTSET = Mask;
			*Action = LED_NO_ACTION;
			break;

		case LED_TOGGLE:
			LED_PORT.OUTTGL = Mask;
			*Action = LED_NO_ACTION;
			break;

		case LED_PULSE:
			if (!(LED_PORT.OUT & Mask)) {
				LED_PORT.OUTSET = Mask;
			} else {
				LED_PORT.OUTCLR = Mask;
				*Action = LED_NO_ACTION;
			}
			break;

		case LED_BLINK_1X ... LED_BLINK_8X:
			if (++BlinkPrescaler == BLINK_PRESCALER) {
				BlinkPrescaler = 0;
				/* Blink functionality occurs at slower speed than Tick-frequency */

				if (!(LED_PORT.OUT & Mask)) {
					/* LED is off, turn it on */
					LED_PORT.OUTSET = Mask;
				} else {
					/* LED is on, turn it off and change state */
					LED_PORT.OUTCLR = Mask;

					if (*Action == LED_BLINK_1X) {
						*Action = LED_NO_ACTION;
					} else {
						/* Still some blinks to do. Use the fact that LED_BLINK_XY are ordered sequentially */
						*Action = *Action - 1;
					}
				}
			}
			break;

		default:
			/* Should not happen (TM) */
			*Action = LED_NO_ACTION;
			break;

	}
}

void LEDInit(void)
{
	LED_PORT.DIRSET = LED_MASK;
}


void LEDTick(void)
{
	Tick(LED_RED, &LEDRedAction);
	Tick(LED_GREEN, &LEDGreenAction);
}

/* TODO: This would be nicer as INLINE */
void LEDTrigger(LEDFunctionEnum Func, LEDActionEnum Action) {
	if (GlobalSettings.ActiveSettingPtr->LEDGreenFunction == Func) {
		LEDGreenAction = Action;
	}

	if (GlobalSettings.ActiveSettingPtr->LEDRedFunction == Func) {
		LEDRedAction = Action;
	}
}

void LEDGetFuncList(char* ListOut, uint16_t BufferSize)
{
    uint8_t i;

    /* Account for '\0' */
    BufferSize--;

    for (i=0; i<LED_FUNC_COUNT; i++) {
        const char* FuncName = LEDFuncTable[i];
        char c;

        while( (c = pgm_read_byte(FuncName)) != '\0' && BufferSize > sizeof(LEDFuncTable[i]) ) {
            /* While not end-of-string and enough buffer to
            * put a complete configuration name */
            *ListOut++ = c;
            FuncName++;
            BufferSize--;
        }

        if ( i < (LED_FUNC_COUNT - 1) ) {
            /* No comma on last configuration */
            *ListOut++ = ',';
            BufferSize--;
        }
    }

    *ListOut = '\0';
}

void LEDSetFuncById(uint8_t Mask, LEDFunctionEnum Func)
{
#ifndef LED_SETTING_GLOBAL
	if (Mask & LED_GREEN) {
		GlobalSettings.ActiveSettingPtr->LEDGreenFunction = Func;
	}

	if (Mask & LED_RED) {
		GlobalSettings.ActiveSettingPtr->LEDRedFunction = Func;
	}
#else
	/* Write LED func to all settings when using global settings */
	for (uint8_t i=0; i<SETTINGS_COUNT; i++) {
		if (Mask & LED_GREEN) {
			GlobalSettings.Settings[i].LEDGreenFunction = Func;
		}

		if (Mask & LED_RED) {
			GlobalSettings.Settings[i].LEDRedFunction = Func;
		}
	}
#endif

	/* Clear modified LED and remove any pending actions */
	if (Mask & LED_GREEN) {
		LED_PORT.OUTCLR = LED_GREEN;
		LEDGreenAction = LED_NO_ACTION;
	}

	if (Mask & LED_RED) {
		LED_PORT.OUTCLR = LED_RED;
		LEDRedAction = LED_NO_ACTION;
	}

}

void LEDGetFuncByName(uint8_t Mask, char* FuncOut, uint16_t BufferSize)
{
	if (Mask == LED_GREEN) {
		strncpy_P(FuncOut, LEDFuncTable[GlobalSettings.ActiveSettingPtr->LEDGreenFunction], BufferSize);
	} else if (Mask == LED_RED) {
		strncpy_P(FuncOut, LEDFuncTable[GlobalSettings.ActiveSettingPtr->LEDRedFunction], BufferSize);
	} else {
		*FuncOut = '\0';
	}
}

bool LEDSetFuncByName(uint8_t Mask, const char* FuncName)
{
    uint8_t i;

    for (i=0; i<LED_FUNC_COUNT; i++) {
        if (strcmp_P(FuncName, LEDFuncTable[i]) == 0) {
            LEDSetFuncById(Mask, i);
            return true;
        }
    }

    /* LED Func not found */
    return false;
}

