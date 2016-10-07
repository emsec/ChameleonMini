#include "Button.h"
#include "Random.h"
#include "Common.h"
#include "Settings.h"
#include "Memory.h"

#define LONG_PRESS_TICK_COUNT	10

static const char PROGMEM ButtonActionTable[][32] =
{
    [BUTTON_ACTION_NONE] = "NONE",
    [BUTTON_ACTION_UID_RANDOM] = "UID_RANDOM",
    [BUTTON_ACTION_UID_LEFT_INCREMENT] = "UID_LEFT_INCREMENT",
    [BUTTON_ACTION_UID_RIGHT_INCREMENT] = "UID_RIGHT_INCREMENT",
    [BUTTON_ACTION_UID_LEFT_DECREMENT] = "UID_LEFT_DECREMENT",
    [BUTTON_ACTION_UID_RIGHT_DECREMENT] = "UID_RIGHT_DECREMENT",
    [BUTTON_ACTION_CYCLE_SETTINGS] = "CYCLE_SETTINGS",
    [BUTTON_ACTION_STORE_MEM] = "STORE_MEM",
    [BUTTON_ACTION_RECALL_MEM] = "RECALL_MEM",
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
	BUTTON_PORT.BUTTON_PINCTRL = PORT_OPC_PULLUP_gc;
}

void ButtonTick(void)
{
    static uint8_t PressTickCounter = 0;
    uint8_t ThisButtonState = ~BUTTON_PORT.IN;

    if (ThisButtonState & BUTTON_MASK) {
    	/* Button is currently pressed */
    	if (PressTickCounter < LONG_PRESS_TICK_COUNT) {
    		/* Count ticks while button is being pressed */
    		PressTickCounter++;
    	} else if (PressTickCounter == LONG_PRESS_TICK_COUNT) {
    		/* Long button press detected execute button action and advance PressTickCounter
    		 * to an invalid state. */
    		ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonLongAction);
    		PressTickCounter++;
    	} else {
    		/* Button is still pressed, ignore */
    	}
    } else if (!(ThisButtonState & BUTTON_MASK)) {
    	/* Button is currently not being pressed. Check if PressTickCounter contains
    	 * a recent short button press. */
    	if ( (PressTickCounter > 0) && (PressTickCounter <= LONG_PRESS_TICK_COUNT) ) {
    		/* We have a short button press */
    		ExecuteButtonAction(GlobalSettings.ActiveSettingPtr->ButtonAction);
    	}

    	PressTickCounter = 0;
    }
}

void ButtonGetActionList(char* ListOut, uint16_t BufferSize)
{
    uint8_t i;

    /* Account for '\0' */
    BufferSize--;

    for (i=0; i<BUTTON_ACTION_COUNT; i++) {
        const char* ActionName = ButtonActionTable[i];
        char c;

        while( (c = pgm_read_byte(ActionName)) != '\0' && BufferSize > sizeof(ButtonActionTable[i]) ) {
            /* While not end-of-string and enough buffer to
            * put a complete configuration name */
            *ListOut++ = c;
            ActionName++;
            BufferSize--;
        }

        if ( i < (BUTTON_ACTION_COUNT - 1) ) {
            /* No comma on last configuration */
            *ListOut++ = ',';
            BufferSize--;
        }
    }

    *ListOut = '\0';
}

void ButtonSetActionById(ButtonTypeEnum Type, ButtonActionEnum Action)
{
#ifndef BUTTON_SETTING_GLOBAL
	if (Type == BUTTON_PRESS_SHORT) {
		GlobalSettings.ActiveSettingPtr->ButtonAction = Action;
	} else if (Type == BUTTON_PRESS_LONG) {
		GlobalSettings.ActiveSettingPtr->ButtonLongAction = Action;
	}
#else
	/* Write button action to all settings when using global settings */
	for (uint8_t i=0; i<SETTINGS_COUNT; i++) {
		if (Type == BUTTON_PRESS_SHORT) {
			GlobalSettings.Settings[i].ButtonAction = Action;
		} else if (Type == BUTTON_PRESS_LONG) {
			GlobalSettings.Settings[i].ButtonLongAction = Action;
		}
	}
#endif
}

void ButtonGetActionByName(ButtonTypeEnum Type, char* ActionOut, uint16_t BufferSize)
{
	if (Type == BUTTON_PRESS_SHORT) {
		strncpy_P(ActionOut, ButtonActionTable[GlobalSettings.ActiveSettingPtr->ButtonAction], BufferSize);
	} else if (Type == BUTTON_PRESS_LONG) {
		strncpy_P(ActionOut, ButtonActionTable[GlobalSettings.ActiveSettingPtr->ButtonLongAction], BufferSize);
	} else {
		/* Should not happen (TM) */
		*ActionOut = '\0';
	}
}

bool ButtonSetActionByName(ButtonTypeEnum Type, const char* Action)
{
    uint8_t i;

    for (i=0; i<BUTTON_ACTION_COUNT; i++) {
        if (strcmp_P(Action, ButtonActionTable[i]) == 0) {
            ButtonSetActionById(Type, i);
            return true;
        }
    }

    /* Button action not found */
    return false;
}
