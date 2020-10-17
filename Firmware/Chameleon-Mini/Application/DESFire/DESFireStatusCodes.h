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

/* 
 * DESFireStatusCodes.h 
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_STATUS_CODES_H__
#define __DESFIRE_STATUS_CODES_H__

#include "DESFireFirmwareSettings.h"

extern BYTE FRAME_CONTINUE[];
extern BYTE OPERATION_OK[];
extern BYTE OK[];
extern BYTE INIT[];

#define DESFIRE_STATUS_RESPONSE_SIZE 1

// NOTE: See p. 72 of the datasheet for descriptions of these response codes: 
typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
    STATUS_OPERATION_OK = 0x00,
    STATUS_NO_CHANGES = 0x0C,
    STATUS_OUT_OF_EEPROM_ERROR = 0x0E,
    STATUS_ILLEGAL_COMMAND_CODE = 0x1C,
    STATUS_INTEGRITY_ERROR = 0x1E,
    STATUS_NO_SUCH_KEY = 0x40,
    STATUS_LENGTH_ERROR = 0x7E,
    STATUS_PERMISSION_DENIED = 0x9D,
    STATUS_PARAMETER_ERROR = 0x9E,
    STATUS_APP_NOT_FOUND = 0xA0,
    STATUS_APP_INTEGRITY_ERROR = 0xA1,
    STATUS_AUTHENTICATION_ERROR = 0xAE,
    STATUS_ADDITIONAL_FRAME = 0xAF,
    STATUS_BOUNDARY_ERROR = 0xBE,
    STATUS_COMMAND_ABORTED = 0xCA,
    STATUS_APP_COUNT_ERROR = 0xCE,
    STATUS_DUPLICATE_ERROR = 0xDE,
    STATUS_EEPROM_ERROR = 0xEE,
    STATUS_FILE_NOT_FOUND = 0xF0,
    STATUS_PICC_INTEGRITY_ERROR = 0xC1,
    STATUS_WRONG_VALUE_ERROR = 0x6E,
} DesfireStatusCodeType;
    
#define SW_NO_ERROR                                    ((uint16_t) 0x9000)
#define SW_BYTES_REMAINING_00                          ((uint16_t) 0x6100)
#define SW_WRONG_LENGTH                                ((uint16_t) 0x6700)
#define SW_SECURITY_STATUS_NOT_SATISFIED               ((uint16_t) 0x6982)
#define SW_FILE_INVALID                                ((uint16_t) 0x6983)
#define SW_DATA_INVALID                                ((uint16_t) 0x6984;
#define SW_CONDITIONS_NOT_SATISFIED                    ((uint16_t) 0x6985)
#define SW_COMMAND_NOT_ALLOWED                         ((uint16_t) 0x6986)
#define SW_APPLET_SELECT_FAILED                        ((uint16_t) 0x6999)
#define SW_WRONG_DATA                                  ((uint16_t) 0x6a80)
#define SW_FUNC_NOT_SUPPORTED                          ((uint16_t) 0x6a81)
#define SW_FILE_NOT_FOUND                              ((uint16_t) 0x6a82)
#define SW_RECORD_NOT_FOUND                            ((uint16_t) 0x6a83)
#define SW_INCORRECT_P1P2                              ((uint16_t) 0x6a86)
#define SW_WRONG_P1P2                                  ((uint16_t) 0x6b00)
#define SW_CORRECT_LENGTH_00                           ((uint16_t) 0x6c00)
#define SW_INS_NOT_SUPPORTED                           ((uint16_t) 0x6d00)
#define SW_CLA_NOT_SUPPORTED                           ((uint16_t) 0x6e00)
#define SW_UNKNOWN                                     ((uint16_t) 0x6f00)
#define SW_FILE_FULL                                   ((uint16_t) 0x6a84)
#define SW_LOGICAL_CHANNEL_NOT_SUPPORTED               ((uint16_t) 0x6881)
#define SW_SECURE_MESSAGING_NOT_SUPPORTED              ((uint16_t) 0x6882)
#define SW_WARNING_STATE_UNCHANGED                     ((uint16_t) 0x6200)
#define SW_REFERENCE_DATA_NOT_FOUND                    ((uint16_t) 0x6a88)	
#define SW_INTERNAL_ERROR                              ((uint16_t) 0x6d66)

#endif
