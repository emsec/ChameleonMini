#include "LED.h"
#include "Settings.h"
#include "Map.h"

#include "Terminal/CommandLine.h"
#include "System.h"

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
    { .Id = LED_CODEC_TX,		.Text = "CODEC_TX"			},
    { .Id = LED_FIELD_DETECTED,	.Text = "FIELD_DETECTED"	},
    { .Id = LED_LOG_MEM_FULL,	.Text = "LOGMEM_FULL"		},
};

#ifdef CHAMELEON_TINY_SUPPORT
// Code below imported from RfidResearchGroup/ChameleonMini and authored by
// @willok
uint8_t LedConfig[] = {
    1,    0,    2,    2,    2,        //    1
    0,    1,    2,    2,    2,
    0,    0,    1,    2,    2,
    0,    0,    0,    2,    2,
    0,    0,    2,    1,    2,
    0,    0,    2,    0,    2,
    0,    0,    2,    2,    1,
    0,    0,    2,    2,    0,
    0,    0,    0,    1,    2,        //    9
};

void Led8Map(uint8_t LedNo) {
    if (LedNo > SETTINGS_COUNT) {
        return;
    }

    if (LedConfig[LedNo * 5])
        LED_PORT.OUTSET = PIN4_bm;
    else
        LED_PORT.OUTCLR = PIN4_bm;

    if (LedConfig[LedNo * 5 + 1])
        LED_PORT.OUTSET = PIN3_bm;
    else
        LED_PORT.OUTCLR = PIN3_bm;

    if (0 == LedConfig[LedNo * 5 + 2]) {
        PORTA.OUTCLR = PIN0_bm;
        PORTA.DIRSET = PIN0_bm;
    } else if (1 == LedConfig[LedNo * 5 + 2]) {
        PORTA.OUTSET = PIN0_bm;
        PORTA.DIRSET = PIN0_bm;
    } else {
        PORTA.DIRCLR = PIN0_bm;
    }

    if (0 == LedConfig[LedNo * 5 + 3]) {
        PORTE.OUTCLR = PIN1_bm;
        PORTE.DIRSET = PIN1_bm;
    } else if (1 == LedConfig[LedNo * 5 + 3]) {
        PORTE.OUTSET = PIN1_bm;
        PORTE.DIRSET = PIN1_bm;
    } else {
        PORTE.DIRCLR = PIN1_bm;
    }

    if (0 == LedConfig[LedNo * 5 + 4]) {
        PORTE.OUTCLR = PIN0_bm;
        PORTE.DIRSET = PIN0_bm;
    } else if (1 == LedConfig[LedNo * 5 + 4]) {
        PORTE.OUTSET = PIN0_bm;
        PORTE.DIRSET = PIN0_bm;
    } else {
        PORTE.DIRCLR = PIN0_bm;
    }
}

#endif


INLINE void Tick(uint8_t Mask, LEDActionEnum *Action) {
    static uint8_t LEDRedBlinkPrescaler = 0;
    static uint8_t LEDGreenBlinkPrescaler = 0;
    uint8_t *BlinkPrescaler = (Action == &LEDGreenAction) ? &LEDGreenBlinkPrescaler : &LEDRedBlinkPrescaler;

    switch (*Action) {
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
            if (++(*BlinkPrescaler) == BLINK_PRESCALER) {
                *BlinkPrescaler = 0;

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

void LEDInit(void) {
    LED_PORT.DIRSET = LED_MASK;
}


void LEDTick(void) {
#ifndef CHAMELEON_TINY_SUPPORT
    Tick(LED_RED, &LEDRedAction);
    Tick(LED_GREEN, &LEDGreenAction);
#else
// Code below imported from RfidResearchGroup/ChameleonMini and authored by
// @willok
    static uint8_t LastLed = 0xFF;
    if (LastLed != GlobalSettings.ActiveSettingIdx) {
	LastLed = GlobalSettings.ActiveSettingIdx;
	Led8Map(LastLed);
    }
#endif
}

void LEDGetFuncList(char *List, uint16_t BufferSize) {
    MapToString(LEDFunctionMap, ARRAY_COUNT(LEDFunctionMap), List, BufferSize);
}

void LEDSetFuncById(uint8_t Mask, LEDHookEnum Function) {
#ifndef LED_SETTING_GLOBAL
    if (Mask & LED_GREEN) {
        GlobalSettings.ActiveSettingPtr->LEDGreenFunction = Function;
    }

    if (Mask & LED_RED) {
        GlobalSettings.ActiveSettingPtr->LEDRedFunction = Function;
    }
#else
    /* Write LED func to all settings when using global settings */
    for (uint8_t i = 0; i < SETTINGS_COUNT; i++) {
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

void LEDGetFuncByName(uint8_t Mask, char *Function, uint16_t BufferSize) {
    if (Mask == LED_GREEN) {
        MapIdToText(LEDFunctionMap, ARRAY_COUNT(LEDFunctionMap),
                    GlobalSettings.ActiveSettingPtr->LEDGreenFunction, Function, BufferSize);
    } else if (Mask == LED_RED) {
        MapIdToText(LEDFunctionMap, ARRAY_COUNT(LEDFunctionMap),
                    GlobalSettings.ActiveSettingPtr->LEDRedFunction, Function, BufferSize);
    }
}

bool LEDSetFuncByName(uint8_t Mask, const char *Function) {
    MapIdType Id;

    if (MapTextToId(LEDFunctionMap, ARRAY_COUNT(LEDFunctionMap), Function, &Id)) {
        LEDSetFuncById(Mask, Id);
        return true;
    } else {
        return false;
    }
}

