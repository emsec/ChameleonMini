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
 * DESFireInstructions.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_INS_COMMANDS_H__
#define __DESFIRE_INS_COMMANDS_H__

#include "DESFireFirmwareSettings.h"
#include "DESFireCrypto.h"

#define DESFIRE_VERSION1_BYTES_PROCESSED     (8)
#define DESFIRE_VERSION2_BYTES_PROCESSED     (8)
#define DESFIRE_VERSION3_BYTES_PROCESSED     (15)

typedef struct DESFIRE_FIRMWARE_PACKING {
    uint8_t NextIndex;
    uint8_t CryptoMethodType;
    uint8_t ActiveCommMode;
    uint8_t KeyId;
    uint8_t RndB[CRYPTO_CHALLENGE_RESPONSE_BYTES] DESFIRE_FIRMWARE_ARRAY_ALIGNAT;
} DesfireSavedCommandStateType;
extern DesfireSavedCommandStateType DesfireCommandState;

typedef struct DESFIRE_FIRMWARE_PACKING {
    BYTE BytesProcessed;
    BOOL IsComplete;
} TransferStatus;

typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {

    /* DESFire native command support: */
    NO_COMMAND_TO_CONTINUE = 0x00,
    CMD_AUTHENTICATE = 0x0A,               /* Authenticate Legacy */
    CMD_AUTHENTICATE_ISO = 0x1A,
    CMD_AUTHENTICATE_AES = 0xAA,
    CMD_AUTHENTICATE_EV2_FIRST = 0x71,     /* See page 32 of AN12343.pdf */
    CMD_AUTHENTICATE_EV2_NONFIRST = 0x77,  /* See page 32 of AN12343.pdf */
    CMD_CHANGE_KEY_SETTINGS = 0x54,
    CMD_SET_CONFIGURATION =  0x5C,
    CMD_CHANGE_KEY =  0xC4,
    CMD_GET_KEY_VERSION = 0x64,
    CMD_CREATE_APPLICATION =  0xCA,
    CMD_DELETE_APPLICATION =  0xDA,
    CMD_GET_APPLICATION_IDS = 0x6A,
    CMD_FREE_MEMORY = 0x6E,
    CMD_GET_DF_NAMES = 0x6D,
    CMD_GET_KEY_SETTINGS = 0x45,
    CMD_SELECT_APPLICATION =  0x5A,
    CMD_FORMAT_PICC =  0xFC,
    CMD_GET_VERSION = 0x60,
    CMD_GET_CARD_UID = 0x51,
    CMD_GET_FILE_IDS =  0x6F,
    CMD_GET_FILE_SETTINGS = 0xF5,
    CMD_CHANGE_FILE_SETTINGS = 0x5F,
    CMD_CREATE_STDDATA_FILE =  0xCD,
    CMD_CREATE_BACKUPDATA_FILE =  0xCB,
    CMD_CREATE_VALUE_FILE =  0xCC,
    CMD_CREATE_LINEAR_RECORD_FILE = 0xC1,
    CMD_CREATE_CYCLIC_RECORD_FILE = 0xC0,
    CMD_DELETE_FILE = 0xDF,
    CMD_GET_ISO_FILE_IDS = 0x61,
    CMD_READ_DATA =  0xBD,
    CMD_WRITE_DATA =  0x3D,
    CMD_GET_VALUE = 0x6C,
    CMD_CREDIT = 0x0C,
    CMD_DEBIT = 0xDC,
    CMD_LIMITED_CREDIT = 0x1C,
    CMD_WRITE_RECORD = 0x3B,
    CMD_READ_RECORDS = 0xBB,
    CMD_CLEAR_RECORD_FILE = 0xEB,
    CMD_COMMIT_TRANSACTION = 0xC7,
    CMD_ABORT_TRANSACTION = 0xA7,
    CMD_CONTINUE =  0xAF,

    /* ISO7816 Command Set Support: */
    CMD_ISO7816_SELECT = 0xA4,
    CMD_ISO7816_GET_CHALLENGE = 0x84,
    CMD_ISO7816_EXTERNAL_AUTHENTICATE = 0x82,
    CMD_ISO7816_INTERNAL_AUTHENTICATE = 0x88,
    CMD_ISO7816_READ_BINARY = 0xB0,
    CMD_ISO7816_UPDATE_BINARY = 0xD6,
    CMD_ISO7816_READ_RECORDS = 0xB2,
    CMD_ISO7816_APPEND_RECORD = 0xE2,

    /* Space for undocumented command codes --
     * Need command codes and parameters to make these work moving forward: */
    //CMD_READ_SIGNATURE /* See page 87 of AN12343.pdf (for Mifare DESFire Light tags) */

} DESFireCommandType;

typedef uint16_t (*InsCodeHandlerFunc)(uint8_t *Buffer, uint16_t ByteCount);

typedef struct {
    DESFireCommandType  insCode;
    InsCodeHandlerFunc  insFunc;
    const __flash char *insDesc;
} DESFireCommand;

extern const __flash DESFireCommand DESFireCommandSet[];

/* Helper and batch process functions */
uint16_t CallInstructionHandler(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ExitWithStatus(uint8_t *Buffer, uint8_t StatusCode, uint16_t DefaultReturnValue);
uint16_t CmdNotImplemented(uint8_t *Buffer, uint16_t ByteCount);

/*
 * The following section implements:
 * DESFire EV0 / D40 specific commands
 */

/* General commands */
uint16_t EV0CmdGetVersion1(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdGetVersion2(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdGetVersion3(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdFormatPicc(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdGetCardUID(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdSetConfiguration(uint8_t *Buffer, uint16_t ByteCount); // ?? Docs ??
uint16_t DesfireCmdFreeMemory(uint8_t *Buffer, uint16_t ByteCount); // returns free memory on the tag

/* Key management commands */
uint16_t EV0CmdChangeKey(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdGetKeySettings(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdChangeKeySettings(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdGetKeyVersion(uint8_t *Buffer, uint16_t ByteCount);

/* Application management commands */
uint16_t EV0CmdGetApplicationIds1(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdCreateApplication(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdDeleteApplication(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdSelectApplication(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdGetDFNames(uint8_t *Buffer, uint16_t ByteCount);

/* File management commands */
uint16_t EV0CmdCreateStandardDataFile(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdCreateBackupDataFile(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdCreateValueFile(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdCreateLinearRecordFile(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdCreateCyclicRecordFile(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdDeleteFile(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdGetFileIds(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdGetFileSettings(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdChangeFileSettings(uint8_t *Buffer, uint16_t ByteCount);

/* Data manipulation commands */
// NOTE: Page 57: Read file functions:
uint16_t EV0CmdReadData(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdWriteData(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdGetValue(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdCredit(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdDebit(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdLimitedCredit(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdReadRecords(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdWriteRecord(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdClearRecords(uint8_t *Buffer, uint16_t ByteCount);

/* Transaction handling commands */
uint16_t EV0CmdCommitTransaction(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdAbortTransaction(uint8_t *Buffer, uint16_t ByteCount);

/* EV1/EV2 supported commands */
uint16_t EV0CmdAuthenticateLegacy1(uint8_t *Buffer, uint16_t ByteCount);
uint16_t EV0CmdAuthenticateLegacy2(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdAuthenticate3KTDEA1(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdAuthenticate3KTDEA2(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdAuthenticateAES1(uint8_t *Buffer, uint16_t ByteCount);
uint16_t DesfireCmdAuthenticateAES2(uint8_t *Buffer, uint16_t ByteCount);

/* ISO7816 command handling */
uint16_t ISO7816CmdSelect(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdSelectEF(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdSelectDF(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdGetChallenge(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdExternalAuthenticate(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdInternalAuthenticate(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdReadBinary(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdUpdateBinary(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdReadRecords(uint8_t *Buffer, uint16_t ByteCount);
uint16_t ISO7816CmdAppendRecord(uint8_t *Buffer, uint16_t ByteCount);

#endif
