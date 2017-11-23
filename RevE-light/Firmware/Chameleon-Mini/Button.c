#include "Button.h"
#include "Random.h"
#include "Common.h"
#include "Settings.h"
#include "Memory.h"
#include "Map.h"
#include "Application/Application.h"

#define BUTTON_PORT     	PORTA
#define BUTTON_L			PIN3_bm
#define BUTTON_R			PIN6_bm
#define BUTTON_L_PINCTRL	PIN3CTRL
#define BUTTON_R_PINCTRL	PIN6CTRL
#define BUTTON_MASK     	(BUTTON_L | BUTTON_R)

#define LONG_PRESS_TICK_COUNT	10

static const MapEntryType PROGMEM ButtonActionMap[] = {
	{ .Id = BUTTON_ACTION_NONE,					.Text = "NONE" },
	{ .Id = BUTTON_ACTION_UID_RANDOM,			.Text = "UID_RANDOM" },
	{ .Id = BUTTON_ACTION_UID_LEFT_INCREMENT,	.Text = "UID_LEFT_INCREMENT" },
	{ .Id = BUTTON_ACTION_UID_RIGHT_INCREMENT,	.Text = "UID_RIGHT_INCREMENT" },
	{ .Id = BUTTON_ACTION_UID_LEFT_DECREMENT,	.Text = "UID_LEFT_DECREMENT" },
	{ .Id = BUTTON_ACTION_UID_RIGHT_DECREMENT,	.Text = "UID_RIGHT_DECREMENT" },
	{ .Id = BUTTON_ACTION_CYCLE_SETTINGS,		.Text = "CYCLE_SETTINGS" },
	{ .Id = BUTTON_ACTION_STORE_MEM,			.Text = "STORE_MEM" },
	{ .Id = BUTTON_ACTION_RECALL_MEM,			.Text = "RECALL_MEM" },
};

static void ExecuteButtonAction(ButtonActionEnum ButtonAction)
{
    uint8_t UidBuffer[32];

    if (ButtonAction == BUTTON_ACTION_UID_RANDOM) {
        for (uint8_t i=0; i<ActiveConfiguration.UidSize; i++) {
            UidBuffer[i] = RandomGetByte();
        }

        ApplicationSetUid(UidBuffer);
    } else if (ButtonAction == BUTTON_ACTION_UID_LEFT_INCREMENT) {
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i;

        for (i=0; i<ActiveConfiguration.UidSize; i++) {
            if (Carry) {
                if (UidBuffer[i] == 0xFF) {
                    Carry = 1;
                } else {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] + 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
    } else if (ButtonAction == BUTTON_ACTION_UID_RIGHT_INCREMENT) {
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i = ActiveConfiguration.UidSize;

        while(i-- > 0) {
            if (Carry) {
                if (UidBuffer[i] == 0xFF) {
                    Carry = 1;
                } else {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] + 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
    } else if (ButtonAction == BUTTON_ACTION_UID_LEFT_DECREMENT) {
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i;

        for (i=0; i<ActiveConfiguration.UidSize; i++) {
            if (Carry) {
                if (UidBuffer[i] == 0x00) {
                    Carry = 1;
                } else {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] - 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
    } else if (ButtonAction == BUTTON_ACTION_UID_RIGHT_DECREMENT) {
        ApplicationGetUid(UidBuffer);
        bool Carry = 1;
        uint8_t i = ActiveConfiguration.UidSize;

        while(i-- > 0) {
            if (Carry) {
                if (UidBuffer[i] == 0x00) {
                    Carry = 1;
                } else {
                    Carry = 0;
                }

                UidBuffer[i] = (UidBuffer[i] - 1) & 0xFF;
            }
        }

        ApplicationSetUid(UidBuffer);
    } else if (ButtonAction == BUTTON_ACTION_CYCLE_SETTINGS) {
    	SettingsCycle();
    } else if (ButtonAction == BUTTON_ACTION_STORE_MEM) {
    	MemoryStore();
    } else if (ButtonAction == BUTTON_ACTION_RECALL_MEM) {
    	MemoryRecall();
    }
}

void ButtonInit(void)
{
	BUTTON_PORT.DIRCLR = BUTTON_MASK;
	BUTTON_PORT.BUTTON_R_PINCTRL = PORT_OPC_PULLUP_gc;
	BUTTON_PORT.BUTTON_L_PINCTRL = PORT_OPC_PULLUP_gc;
}

void ButtonTick(void)
{
    static uint8_t ButtonRPressTick = 0;
    static uint8_t ButtonLPressTick = 0;
    uint8_t ThisButtonState = ~BUTTON_PORT.IN;

    if (ThisButtonState & BUTTON_R) {
    	/* Button is currently pressed */
    	if (ButtonRPressTick < LONG_PRESS_TICK_COUNT) {
    		/* Count ticks while button is being pressed */
    		ButtonRPressTick++;
    	} else if (ButtonRPressTick == LONG_PRESS_TICK_COUNT) {
    		/* Long button press detected execute button action and advance PressTickCounter
    		 * to an invalid state. */
    		ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_R_PRESS_LONG]);
    		ButtonRPressTick++;
    	} else {
    		/* Button is still pressed, ignore */
    	}
    } else if (!(ThisButtonState & BUTTON_MASK)) {
    	/* Button is currently not being pressed. Check if PressTickCounter contains
    	 * a recent short button press. */
    	if ( (ButtonRPressTick > 0) && (ButtonRPressTick <= LONG_PRESS_TICK_COUNT) ) {
    		/* We have a short button press */
    		ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_R_PRESS_SHORT]);
    	}

    	ButtonRPressTick = 0;
    }

    if (ThisButtonState & BUTTON_L) {
    	/* Button is currently pressed */
    	if (ButtonLPressTick < LONG_PRESS_TICK_COUNT) {
    		/* Count ticks while button is being pressed */
    		ButtonLPressTick++;
    	} else if (ButtonLPressTick == LONG_PRESS_TICK_COUNT) {
    		/* Long button press detected execute button action and advance PressTickCounter
    		 * to an invalid state. */
    		ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_L_PRESS_LONG]);
    		ButtonLPressTick++;
    	} else {
    		/* Button is still pressed, ignore */
    	}
    } else if (!(ThisButtonState & BUTTON_MASK)) {
    	/* Button is currently not being pressed. Check if PressTickCounter contains
    	 * a recent short button press. */
    	if ( (ButtonLPressTick > 0) && (ButtonLPressTick <= LONG_PRESS_TICK_COUNT) ) {
    		/* We have a short button press */
    		ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonActions[BUTTON_L_PRESS_SHORT]);
    	}

    	ButtonLPressTick = 0;
    }
}

void ButtonGetActionList(char* List, uint16_t BufferSize)
{
	MapToString(ButtonActionMap, sizeof(ButtonActionMap)/sizeof(*ButtonActionMap), List, BufferSize);
}

void ButtonSetActionById(ButtonTypeEnum Type, ButtonActionEnum Action)
{
#ifndef BUTTON_SETTING_GLOBAL
	GlobalSettings.ActiveSettingPtr->ButtonActions[Type] = Action;
#else
	/* Write button action to all settings when using global settings */
	for (uint8_t i=0; i<SETTINGS_COUNT; i++) {
		GlobalSettings.Settings[i].ButtonActions[Type] = Action;
	}
#endif
}

void ButtonGetActionByName(ButtonTypeEnum Type, char* Action, uint16_t BufferSize)
{
	MapIdToText(ButtonActionMap, sizeof(ButtonActionMap)/sizeof(*ButtonActionMap),
			GlobalSettings.ActiveSettingPtr->ButtonActions[Type], Action, BufferSize);
}

bool ButtonSetActionByName(ButtonTypeEnum Type, const char* Action)
{
	MapIdType Id;

	if (MapTextToId(ButtonActionMap, sizeof(ButtonActionMap)/sizeof(*ButtonActionMap),
			Action, &Id)) {
		ButtonSetActionById(Type, Id);
		return true;
	} else {
		return false;
	}
}
