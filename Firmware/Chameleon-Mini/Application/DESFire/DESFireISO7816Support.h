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

/* DESFireISO7816Support.h */

#ifndef __DESFIRE_ISO7816_SUPPORT_H__
#define __DESFIRE_ISO7816_SUPPORT_H__

#include <inttypes.h>
#include <stdbool.h>

#define Iso7816CLA(cmdCode) \
    (cmdCode == DESFIRE_ISO7816_CLA)

#define ISO7816_PROLOGUE_SIZE                        (2)
#define ISO7816_STATUS_RESPONSE_SIZE                 (0x02)
#define ISO7816_EF_NOT_SPECIFIED                     (0xff)
#define ISO7816_EFID_NUMBER_MAX                      (0x0f)
#define ISO7816_MAX_FILE_SIZE                        (0xfd)
#define ISO7816_READ_ALL_BYTES_SIZE                  (0x00)
#define ISO7816_CMD_NO_ERROR                         (0x00)
#define ISO7816_ERROR_SW1                            (0x6a)
#define ISO7816_ERROR_SW1_INS_UNSUPPORTED            (0x6d)
#define ISO7816_ERROR_SW1_ACCESS                     (0x69)
#define ISO7816_ERROR_SW1_FSE                        (0x62)
#define ISO7816_ERROR_SW1_WRONG_FSPARAMS             (0x6b)
#define ISO7816_ERROR_SW2_INCORRECT_P1P2             (0x86)
#define ISO7816_ERROR_SW2_UNSUPPORTED                (0x81)
#define ISO7816_ERROR_SW2_FUNC_UNSUPPORTED           (0x81)
#define ISO7816_ERROR_SW2_INS_UNSUPPORTED            (0x00)
#define ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED    (0x81)
#define ISO7816_SELECT_ERROR_SW2_NOFILE              (0x82)
#define ISO7816_ERROR_SW2_FILE_NOMEM                 (0x84)
#define ISO7816_GET_CHALLENGE_ERROR_SW2_UNSUPPORTED  (0x81)
#define ISO7816_ERROR_SW2_INCOMPATFS                 (0x81)
#define ISO7816_ERROR_SW2_SECURITY                   (0x82)
#define ISO7816_ERROR_SW2_NOEF                       (0x86)
#define ISO7816_ERROR_SW2_WRONG_FSPARAMS             (0x00)
#define ISO7816_ERROR_SW2_EOF                        (0x82)

#define AppendSW12Bytes(sw1, sw2)   \
    ((uint16_t)  ((sw1 << 8) | (sw2 & 0xff)))

/* Some of the wrapped ISO7816 commands have extra meaning
 * packed into the P1-P2 bytes of the APDU byte array.
 * When we support these extra modes, this is a way to keep
 * track of the local meanings without needing extra handling
 * functions to distinguish between the wrapped command types
 * for the ISO7816 versus native DESFire instructions.
 */
typedef enum {
    ISO7816_NO_DATA = 0,
    ISO7816_UNSUPPORTED_MODE,
    ISO7816_SELECT_EF,
    ISO7816_SELECT_DF,
    ISO7816_FILE_FIRST_RECORD,
    ISO7816_FILE_LAST_RECORD,
    ISO7816_FILE_NEXT_RECORD,
    ISO7816_FILE_PREV_RECORD,
} Iso7816WrappedParams_t;

extern Iso7816WrappedParams_t Iso7816P1Data;
extern Iso7816WrappedParams_t Iso7816P2Data;
extern bool Iso7816FileSelected;
extern uint8_t Iso7816FileOffset;
extern uint8_t Iso7816EfIdNumber;

bool IsWrappedISO7816CommandType(uint8_t *Buffer, uint16_t ByteCount);
uint16_t SetIso7816WrappedParametersType(uint8_t *Buffer, uint16_t ByteCount);

#endif
