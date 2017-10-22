/*
 * Button.h
 *
 *  Created on: 13.05.2013
 *      Author: skuser
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "Common.h"
#include <avr/io.h>
#include "Application/Application.h"

#define BUTTON_PORT     PORTA
#define BUTTON_MASK     PIN6_bm
#define BUTTON_PINCTRL	PIN6CTRL

typedef enum {
	BUTTON_PRESS_SHORT,
	BUTTON_PRESS_LONG
} ButtonTypeEnum;

typedef enum {
    BUTTON_ACTION_NONE,
    BUTTON_ACTION_UID_RANDOM,
    BUTTON_ACTION_UID_LEFT_INCREMENT,
    BUTTON_ACTION_UID_RIGHT_INCREMENT,
    BUTTON_ACTION_UID_LEFT_DECREMENT,
    BUTTON_ACTION_UID_RIGHT_DECREMENT,
    BUTTON_ACTION_CYCLE_SETTINGS,
    BUTTON_ACTION_STORE_MEM,
    BUTTON_ACTION_RECALL_MEM,

    /* This has to be last element */
    BUTTON_ACTION_COUNT
} ButtonActionEnum;

void ButtonInit(void);
void ButtonTick(void);

void ButtonGetActionList(char* ListOut, uint16_t BufferSize);
void ButtonSetActionById(ButtonTypeEnum Type, ButtonActionEnum Action);
void ButtonGetActionByName(ButtonTypeEnum Type, char* ActionOut, uint16_t BufferSize);
bool ButtonSetActionByName(ButtonTypeEnum Type, const char* Action);

#endif /* BUTTON_H_ */
