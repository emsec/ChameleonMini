/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
 */

#include "Button.h"
#include "Random.h"
#include "Common.h"
#include "Settings.h"

static const char PROGMEM ButtonActionTable[][32] =
{
    [BUTTON_ACTION_NONE] = "NONE",
    [BUTTON_ACTION_UID_RANDOM] = "UID_RANDOM",
    [BUTTON_ACTION_UID_LEFT_INCREMENT] = "UID_LEFT_INCREMENT",
    [BUTTON_ACTION_UID_RIGHT_INCREMENT] = "UID_RIGHT_INCREMENT",
    [BUTTON_ACTION_UID_LEFT_DECREMENT] = "UID_LEFT_DECREMENT",
    [BUTTON_ACTION_UID_RIGHT_DECREMENT] = "UID_RIGHT_DECREMENT"
};

void ButtonInit(void)
{
	BUTTON_PORT.DIRCLR = BUTTON_MASK;
	BUTTON_PORT.BUTTON_PINCTRL = PORT_OPC_PULLUP_gc;
}

void ButtonTick(void)
{
    static uint8_t LastButtonState = 0;
    uint8_t ThisButtonState = ~BUTTON_PORT.IN;
    uint8_t ThisButtonChange = ThisButtonState ^ LastButtonState;
    uint8_t ThisButtonPress = ThisButtonChange & ThisButtonState;
    LastButtonState = ThisButtonState;

    if ( ThisButtonPress & BUTTON_MASK ) {
        uint8_t UidBuffer[32];
        ButtonActionEnum ButtonAction = GlobalSettings.ActiveSettingPtr->ButtonAction;

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
        }
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

void ButtonSetActionById(ButtonActionEnum Action)
{
	GlobalSettings.ActiveSettingPtr->ButtonAction = Action;
}

void ButtonGetActionByName(char* ActionOut, uint16_t BufferSize)
{
    strncpy_P(ActionOut, ButtonActionTable[GlobalSettings.ActiveSettingPtr->ButtonAction], BufferSize);
}

bool ButtonSetActionByName(const char* Action)
{
    uint8_t i;

    for (i=0; i<BUTTON_ACTION_COUNT; i++) {
        if (strcmp_P(Action, ButtonActionTable[i]) == 0) {
            ButtonSetActionById(i);
            return true;
        }
    }

    /* Button action not found */
    return false;
}
