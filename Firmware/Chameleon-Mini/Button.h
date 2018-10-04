/*
 * Button.h
 *
 *  Created on: 13.05.2013
 *      Author: skuser
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "Common.h"

typedef enum {
    BUTTON_R_PRESS_SHORT = 0,
    BUTTON_R_PRESS_LONG,
    BUTTON_L_PRESS_SHORT,
    BUTTON_L_PRESS_LONG,

    /* Must be last element */
    BUTTON_TYPE_COUNT
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
    BUTTON_ACTION_TOGGLE_FIELD,
    BUTTON_ACTION_STORE_LOG,
    BUTTON_ACTION_CLONE,

    /* This has to be last element */
    BUTTON_ACTION_COUNT
} ButtonActionEnum;

void ButtonInit(void);
void ButtonTick(void);

void ButtonGetActionList(char* List, uint16_t BufferSize);
void ButtonSetActionById(ButtonTypeEnum Type, ButtonActionEnum Action);
void ButtonGetActionByName(ButtonTypeEnum Type, char* Action, uint16_t BufferSize);
bool ButtonSetActionByName(ButtonTypeEnum Type, const char* Action);

#endif /* BUTTON_H_ */
