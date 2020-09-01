#include "Pin.h"
#include "Settings.h"
#include "Map.h"

#include "Terminal/CommandLine.h"
#include "System.h"

PinFunctionEnum PinFunction = PIN_NO_FUNC;

static const MapEntryType PROGMEM PinFunctionMap[] = {
    { .Id = PIN_NO_FUNC,		.Text = "NONE" 				},
    { .Id = PIN_ON,				.Text = "ON" 				},
    { .Id = PIN_OFF,			.Text = "OFF"				},
    { .Id = PIN_1_TICK,			.Text = "1" 				},
    { .Id = PIN_2_TICKS,		.Text = "2" 				},
    { .Id = PIN_3_TICKS, 		.Text = "3"					},
    { .Id = PIN_4_TICKS,		.Text = "4"					},
    { .Id = PIN_5_TICKS,		.Text = "5"					},
};

INLINE void Tick(PinFunctionEnum *Action) {
    if (*Action > PIN_OFF) {
        /* Still some time to wait. Use the fact that PIN_XY_TICK(S) are ordered sequentially */
        *Action = *Action - 1;
        if (*Action == PIN_OFF) {
            PIN_PORT.OUTCLR = PIN_0;
        }
    }
}

void PinInit(void) {
    // nothing to do
}

void PinTick(void) {
    Tick(&PinFunction);
}

void PinGetFuncList(char *List, uint16_t BufferSize) {
    MapToString(PinFunctionMap, ARRAY_COUNT(PinFunctionMap), List, BufferSize);
}

void PinSetFuncById(PinFunctionEnum Function) {
    PinFunction = Function;

    if (Function > PIN_NO_FUNC) {
        PIN_PORT.DIRSET = PIN_0;
        if (Function == PIN_OFF) {
            PIN_PORT.OUTCLR = PIN_0;
        } else {
            PIN_PORT.OUTSET = PIN_0;
        }
    } else {
        PIN_PORT.DIRCLR = PIN_0;
    }
}

void PinGetFuncByName(char *Function, uint16_t BufferSize) {
    MapIdToText(PinFunctionMap, ARRAY_COUNT(PinFunctionMap), PinFunction, Function, BufferSize);
}

bool PinSetFuncByName(const char *Function) {
    MapIdType Id;

    if (MapTextToId(PinFunctionMap, ARRAY_COUNT(PinFunctionMap), Function, &Id)) {
        PinSetFuncById(Id);
        return true;
    } else {
        return false;
    }
}

