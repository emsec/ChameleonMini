/*
The DESFire stack portion of this firmware source
is free software written by Maxie Dion Schmidt (@maxieds):
You can redistribute it and/or modify
it under the terms of this license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

The complete source distribution of
this firmware is available at the following link:
https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.

Based in part on the original DESFire code created by
@dev-zzo (GitHub handle) [Dmitry Janushkevich] available at
https://github.com/dev-zzo/ChameleonMini/tree/desfire.

This notice must be retained at the top of all source files where indicated.
*/

/* DESFireISO7816Support.c */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "DESFirePICCHeaderLayout.h"
#include "DESFireISO7816Support.h"
#include "DESFireInstructions.h"
#include "DESFireStatusCodes.h"
#include "../ISO14443-3A.h"

/* Data for the NDEF Tag Application / Mifare DESFire Tag Application in the table:
 * https://www.eftlab.com/knowledge-base/211-emv-aid-rid-pix/
 */
const uint8_t MIFARE_DESFIRE_TAG_AID[9] = {
    0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x00
};

Iso7816WrappedParams_t Iso7816P1Data = ISO7816_NO_DATA;
Iso7816WrappedParams_t Iso7816P2Data = ISO7816_NO_DATA;
bool Iso7816FileSelected = false;
uint8_t Iso7816FileOffset = 0;
uint8_t Iso7816EfIdNumber = ISO7816_EF_NOT_SPECIFIED;

Iso7816WrappedCommandType_t IsWrappedISO7816CommandType(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount >= 6 && Buffer[3] == ByteCount - 6) {
        return ISO7816_WRAPPED_CMD_TYPE_PM3RAW;
    } else if (ByteCount >= 3 && Buffer[2] == STATUS_ADDITIONAL_FRAME) {
        return ISO7816_WRAPPED_CMD_TYPE_PM3_ADDITIONAL_FRAME;
    } else if (ByteCount <= ISO7816_PROLOGUE_SIZE + ISO14443A_CRCA_SIZE + 2) {
        return ISO7816_WRAPPED_CMD_TYPE_NONE;
    } else if (!Iso7816CLA(Buffer[2])) {
        return ISO7816_WRAPPED_CMD_TYPE_NONE;
    } else {
        return ISO7816_WRAPPED_CMD_TYPE_STANDARD;
    }
}

uint16_t SetIso7816WrappedParametersType(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount < 8 || !Iso7816CLA(Buffer[0])) {
        Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
        Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
        return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED);
    } else {
        Iso7816P1Data = ISO7816_NO_DATA;
        Iso7816P2Data = ISO7816_NO_DATA;
    }
    uint8_t insCode = Buffer[1];
    uint8_t P1 = Buffer[2];
    uint8_t P2 = Buffer[3];
    if (insCode == CMD_ISO7816_SELECT) {
        /* Reference: https://cardwerk.com/smart-card-standard-iso7816-4-section-6-basic-interindustry-commands/#chap6_11 */
        if ((P1 & 0xfc) == 0) { // Select by file ID:
            if ((P1 & 0x03) == 0 || (P1 & 0x03) == 1) {
                Iso7816P1Data = ISO7816_SELECT_EF;
            } else {
                Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
                return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED);
            }
        } else if ((P1 & 0xf9) == 0) { // Select by DF/AID name:
            if ((P1 & 0x03) == 0) {
                Iso7816P1Data = ISO7816_SELECT_DF;
            } else {
                Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
                return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED);
            }
        } else {
            Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
            return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED);
        }
        if ((P2 & 0xf0) == 0) {
            switch (P2 & 0x03) {
                case 0:
                    Iso7816P2Data = ISO7816_FILE_FIRST_RECORD;
                    break;
                case 1:
                    Iso7816P2Data = ISO7816_FILE_LAST_RECORD;
                    break;
                case 2:
                    Iso7816P2Data = ISO7816_FILE_NEXT_RECORD;
                    break;
                case 3:
                    Iso7816P2Data = ISO7816_FILE_PREV_RECORD;
                    break;
                default:
                    Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
                    return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED);
            }
        } else {
            Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
            return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED);
        }
    } else if (insCode == CMD_ISO7816_GET_CHALLENGE) {
        if ((P1 != 0) || (P2 != 0)) {
            Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
            Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
            return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED);
        }
        Iso7816P1Data = ISO7816_NO_DATA;
        Iso7816P2Data = ISO7816_NO_DATA;
    } else if (insCode == CMD_ISO7816_READ_BINARY) {
        if ((P1 & 0x80) != 0) {
            if ((P1 & 0x60) != 0) {
                Iso7816P1Data = Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
                return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_ERROR_SW2_INCORRECT_P1P2);
            } else {
                Iso7816EfIdNumber = P1 & ISO7816_EFID_NUMBER_MAX;
                Iso7816FileSelected = false;
                Iso7816FileOffset = P2;
                Iso7816P1Data = Iso7816P2Data = ISO7816_NO_DATA;
            }
        } else {
            Iso7816EfIdNumber = ISO7816_EF_NOT_SPECIFIED;
            Iso7816FileOffset = P1 | P2;
            Iso7816P1Data = Iso7816P2Data = ISO7816_NO_DATA;
        }
    } else if (insCode == CMD_ISO7816_READ_RECORDS) {
        Iso7816FileOffset = 0;
        if (P1 == 0) {
            Iso7816EfIdNumber = ISO7816_EF_NOT_SPECIFIED;
        } else {
            Iso7816EfIdNumber = P1 & ISO7816_EFID_NUMBER_MAX;
            Iso7816FileSelected = false;
        }
        Iso7816P1Data = ISO7816_NO_DATA;
        if ((P2 & 0xf8) != 0) {
            Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
            return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_ERROR_SW2_INCORRECT_P1P2);
        } else if ((P2 & 0x03) == 0) {
            Iso7816P2Data = ISO7816_NO_DATA;
        } else {
            Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
            return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_ERROR_SW2_FUNC_UNSUPPORTED);
        }
    } else if (insCode == CMD_ISO7816_APPEND_RECORD) {
        if ((P1 != 0) || (P2 != 0)) {
            Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
            Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
            return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_ERROR_SW2_FUNC_UNSUPPORTED);
        }
        Iso7816P1Data = Iso7816P2Data = ISO7816_NO_DATA;
    } else if (insCode == CMD_ISO7816_UPDATE_BINARY) {
        if ((P1 & 0x80) != 0) {
            if ((P1 & 0x30) != 0) {
                Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
                Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
                return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_ERROR_SW2_FUNC_UNSUPPORTED);
            }
            Iso7816EfIdNumber = P1 & 0x03;
            Iso7816FileSelected = false;
            Iso7816FileOffset = P2;
            Iso7816P1Data = Iso7816P2Data = ISO7816_NO_DATA;
        } else {
            Iso7816EfIdNumber = ISO7816_EF_NOT_SPECIFIED;
            Iso7816FileOffset = P1 | P2;
            Iso7816P1Data = Iso7816P2Data = ISO7816_NO_DATA;
        }
    }
    //else if(insCode == CMD_ISO7816_EXTERNAL_AUTHENTICATE) {}
    //else if(insCode == CMD_ISO7816_INTERNAL_AUTHENTICATE) {}
    else {
        Iso7816P1Data = ISO7816_UNSUPPORTED_MODE;
        Iso7816P2Data = ISO7816_UNSUPPORTED_MODE;
        return AppendSW12Bytes(ISO7816_ERROR_SW1, ISO7816_ERROR_SW2_FUNC_UNSUPPORTED);
    }
    return ISO7816_CMD_NO_ERROR;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
