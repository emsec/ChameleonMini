/*
 * Pin.h
 *
 *  Created on: 01.09.2020
 *      Author: endres
 */

#ifndef PIN_H
#define PIN_H

#include <avr/io.h>
#include "Common.h"

#define PIN_PORT 		PORTE
#define PIN_0   		PIN0_bm

typedef enum PinFunctionEnum {
    PIN_NO_FUNC = 0,		/* Pin is HI-Z */
    PIN_ON,					/* Pin is ON (3 V) */
    PIN_OFF,				/* Pin is OFF (0 V) */

    /* Have to be sequentially ordered */
    PIN_1_TICK,				/* Pin is ON for 1 Tick, afterwards OFF */
    PIN_2_TICKS,			/* Pin is ON for 2 Ticks, afterwards OFF */
    PIN_3_TICKS,			/* Pin is ON for 3 Ticks, afterwards OFF */
    PIN_4_TICKS,			/* Pin is ON for 4 Ticks, afterwards OFF */
    PIN_5_TICKS,			/* Pin is ON for 5 Ticks, afterwards OFF */

    /* Has to be last element */
    PIN_FUNC_COUNT
} PinFunctionEnum;

void PinInit(void);
void PinTick(void);

void PinGetFuncList(char *List, uint16_t BufferSize);
void PinSetFuncById(PinFunctionEnum Func);
void PinGetFuncByName(char *Function, uint16_t BufferSize);
bool PinSetFuncByName(const char *Function);

#endif /* PIN_H */
