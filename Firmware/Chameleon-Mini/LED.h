/*
 * LED.h
 *
 *  Created on: 10.02.2013
 *      Author: skuser
 */

#ifndef LED_H
#define LED_H

#include <avr/io.h>
#include "Common.h"

#define LED_PORT 		PORTA
#define LED_GREEN		PIN5_bm
#define LED_RED			PIN4_bm
#define LED_MASK		(LED_GREEN | LED_RED)

typedef enum LEDFunctionEnum {
	LED_NO_FUNC = 0,		/* Don't light up the LED */
	LED_TERMINAL_CONN,		/* A Terminal/USB connection has been established */
	LED_TERMINAL_RXTX,		/* There is traffic on the terminal */

	LED_SETTING_CHANGE,		/* Outputs a blink code that shows the current setting */

	LED_MEMORY_STORED, 		/* Blink once when memory has been stored to flash */
	LED_MEMORY_CHANGED, 	/* Switch LED on when card memory has changed compared to flash */

	//TODO: LED_FIELD_DETECTED, /* Shows LED while a reader field is being detected */

	LED_CODEC_RX,			/* Blink LED when receiving codec data */
	LED_CODEC_TX,			/* Blink LED when transmitting codec data */

	//TODO: LED_APP_SELECTED,		/* Show LED while the correct UID has been selected and the application is active */

	/* Has to be last element */
	LED_FUNC_COUNT
} LEDFunctionEnum;

typedef enum LEDActionEnum {
	LED_NO_ACTION = 0x00,
	LED_OFF = 0x10,
	LED_ON = 0x11,
	LED_TOGGLE = 0x12,
	LED_PULSE = 0x13,
	LED_BLINK = 0x20,
	LED_BLINK_1X = 0x20,
	LED_BLINK_2X, /* Have to be sequentially ordered */
	LED_BLINK_3X,
	LED_BLINK_4X,
	LED_BLINK_5X,
	LED_BLINK_6X,
	LED_BLINK_7X,
	LED_BLINK_8X,
} LEDActionEnum;

extern LEDActionEnum LEDGreenAction;
extern LEDActionEnum LEDRedAction;

void LEDInit(void);
void LEDTick(void);

void LEDTrigger(LEDFunctionEnum Func, LEDActionEnum Action);

void LEDGetFuncList(char* ListOut, uint16_t BufferSize);
void LEDSetFuncById(uint8_t Mask, LEDFunctionEnum Func);
void LEDGetFuncByName(uint8_t Mask, char* FuncOut, uint16_t BufferSize);
bool LEDSetFuncByName(uint8_t Mask, const char* FuncName);

#endif /* LED_H */
