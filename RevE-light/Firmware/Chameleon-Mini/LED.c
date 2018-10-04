#include "LED.h"
#include "Settings.h"
#include "Map.h"

#define BLINK_PRESCALER	1 /* x LEDTick(); */

LEDActionEnum LEDGreenAction = LED_NO_ACTION;
LEDActionEnum LEDRedAction = LED_NO_ACTION;

static const MapEntryType PROGMEM LEDFunctionMap[] = {
	{ .Id = LED_NO_FUNC, 		.Text = "NONE" 				},
	{ .Id = LED_POWERED, 		.Text = "POWERED" 			},
	{ .Id = LED_TERMINAL_CONN, 	.Text = "TERMINAL_CONN"		},
	{ .Id = LED_TERMINAL_RXTX,	.Text = "TERMINAL_RXTX" 	},
	{ .Id = LED_SETTING_CHANGE,	.Text = "SETTING_CHANGE" 	},
	{ .Id = LED_MEMORY_STORED, 	.Text = "MEMORY_STORED"		},
	{ .Id = LED_MEMORY_CHANGED,	.Text = "MEMORY_CHANGED"	},
	{ .Id = LED_CODEC_RX,		.Text = "CODEC_RX"			},
	{ .Id = LED_CODEC_TX,		.Text = "CODEC_TX"			}
	//TODO: [LED_FIELD_DETECTED] = "FIELD_DETECTED",
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

void LEDGetFuncList(char* List, uint16_t BufferSize)
{
	MapToString(LEDFunctionMap, sizeof(LEDFunctionMap)/sizeof(*LEDFunctionMap), List, BufferSize);
}

void LEDSetFuncById(uint8_t Mask, LEDHookEnum Function)
{
#ifndef LED_SETTING_GLOBAL
	if (Mask & LED_GREEN) {
		GlobalSettings.ActiveSettingPtr->LEDGreenFunction = Function;
	}

	if (Mask & LED_RED) {
		GlobalSettings.ActiveSettingPtr->LEDRedFunction = Function;
	}
#else
	/* Write LED func to all settings when using global settings */
	for (uint8_t i=0; i<SETTINGS_COUNT; i++) {
		if (Mask & LED_GREEN) {
			GlobalSettings.Settings[i].LEDGreenFunction = Function;
		}

		if (Mask & LED_RED) {
			GlobalSettings.Settings[i].LEDRedFunction = Function;
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

void LEDGetFuncByName(uint8_t Mask, char* Function, uint16_t BufferSize)
{
	if (Mask == LED_GREEN) {
		MapIdToText(LEDFunctionMap, sizeof(LEDFunctionMap)/sizeof(*LEDFunctionMap),
				GlobalSettings.ActiveSettingPtr->LEDGreenFunction, Function, BufferSize);
	} else if (Mask == LED_RED) {
		MapIdToText(LEDFunctionMap, sizeof(LEDFunctionMap)/sizeof(*LEDFunctionMap),
				GlobalSettings.ActiveSettingPtr->LEDRedFunction, Function, BufferSize);
	}
}

bool LEDSetFuncByName(uint8_t Mask, const char* Function)
{
	MapIdType Id;

	if (MapTextToId(LEDFunctionMap, sizeof(LEDFunctionMap)/sizeof(*LEDFunctionMap), Function, &Id)) {
		LEDSetFuncById(Mask, Id);
		return true;
	} else {
		return false;
	}
}

