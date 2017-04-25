/*
 * MifareDesfire.c
 * MIFARE DESFire frontend
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "MifareDesfire.h"
#include "MifareDesfireBE.h"

#include "ISO14443-4.h"
#include "../Codec/ISO14443-2A.h"

/* 
 * Shared variables
 */

uint8_t SessionKey[DESFIRE_CRYPTO_SESSION_KEY_SIZE];
uint8_t SessionIV[DESFIRE_CRYPTO_IV_SIZE];

// #define DEBUG_THIS

#ifdef DEBUG_THIS

#include "../Terminal/Terminal.h"
#include <stdarg.h>

static void DebugPrintP(const char *fmt, ...)
{
    char Format[80];
    char Buffer[80];
    va_list args;

    strcpy_P(Format, fmt);
    va_start(args, fmt);
    vsnprintf(Buffer, sizeof(Buffer), Format, args);
    va_end(args);
    TerminalSendString(Buffer);
}

#define DEBUG_PRINT(fmt, ...) \
    DebugPrintP(PSTR(fmt), ##__VA_ARGS__)

#else /* DEBUG_THIS */

#define DEBUG_PRINT(...)

#endif /* DEBUG_THIS */

/* Anticollision parameters */
#define ATQA_VALUE              0x0344
#define SAK_CL1_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT | ISO14443A_SAK_INCOMPLETE)
#define SAK_CL2_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT)

#define STATUS_FRAME_SIZE           (1 * 8) /* Bits */

#define DESFIRE_EV0_ATS_TL_BYTE 0x06 /* TL: ATS length, 6 bytes */
#define DESFIRE_EV0_ATS_T0_BYTE 0x75 /* T0: TA, TB, TC present; max accepted frame is 64 bytes */
#define DESFIRE_EV0_ATS_TA_BYTE 0x00 /* TA: Only the lowest bit rate is supported */
#define DESFIRE_EV0_ATS_TB_BYTE 0x81 /* TB: taken from the DESFire spec */
#define DESFIRE_EV0_ATS_TC_BYTE 0x02 /* TC: taken from the DESFire spec */

#define GET_LE16(p)     (*(uint16_t*)&(p)[0])
#define GET_LE24(p)     (*(__uint24*)&(p)[0])

/*
 * DESFire-specific things follow
 */

#define ID_PHILIPS_NXP           0x04

/* Defines for GetVersion */
#define DESFIRE_MANUFACTURER_ID         ID_PHILIPS_NXP
/* These do not change */
#define DESFIRE_TYPE                    0x01
#define DESFIRE_SUBTYPE                 0x01
#define DESFIRE_HW_PROTOCOL_TYPE        0x05
#define DESFIRE_SW_PROTOCOL_TYPE        0x05

typedef enum {
    CMD_AUTHENTICATE = 0x0A,
    CMD_CHANGE_KEY_SETTINGS = 0x54,
    CMD_GET_KEY_SETTINGS = 0x45,
    CMD_CHANGE_KEY = 0xC4,
    CMD_GET_KEY_VERSION = 0x64,

    CMD_GET_VERSION = 0x60,

    CMD_CREATE_APPLICATION = 0xCA,
    CMD_DELETE_APPLICATION = 0xDA,
    CMD_GET_APPLICATION_IDS = 0x6A,
    CMD_SELECT_APPLICATION = 0x5A,

    CMD_FORMAT_PICC = 0xFC,

    CMD_GET_FILE_IDS = 0x6F,
    CMD_GET_FILE_SETTINGS = 0xF5,
    CMD_CHANGE_FILE_SETTINGS = 0x5F,
    CMD_CREATE_STANDARD_DATA_FILE = 0xCD,
    CMD_CREATE_BACKUP_DATA_FILE = 0xCB,
    CMD_CREATE_VALUE_FILE = 0xCC,
    CMD_CREATE_LINEAR_RECORD_FILE = 0xC1,
    CMD_CREATE_CYCLIC_RECORD_FILE = 0xC0,
    CMD_DELETE_FILE = 0xDF,
    CMD_READ_DATA = 0xBD,
    CMD_WRITE_DATA = 0x3D,
    CMD_GET_VALUE = 0x6C,
    CMD_CREDIT = 0x0C,
    CMD_DEBIT = 0xDC,
    CMD_LIMITED_CREDIT = 0x1C,
    CMD_WRITE_RECORD = 0x3B,
    CMD_READ_RECORDS = 0xBB,
    CMD_CLEAR_RECORD_FILE = 0xEB,
    CMD_COMMIT_TRANSACTION = 0xC7,
    CMD_ABORT_TRANSACTION = 0xA7,
} DesfireCommandType;

#define DESFIRE_STATUS_RESPONSE_SIZE 1

/*
 * DESFire application code
 */

#define DESFIRE_2KTDEA_NONCE_SIZE CRYPTO_DES_BLOCK_SIZE

/* Authentication status */
#define DESFIRE_MASTER_KEY_ID 0
#define DESFIRE_NOT_AUTHENTICATED 0xFF

typedef enum {
    DESFIRE_AUTH_LEGACY,
    DESFIRE_AUTH_ISO_2KTDEA,
    DESFIRE_AUTH_ISO_3KTDEA,
    DESFIRE_AUTH_AES,
} DesfireAuthType;

typedef union {
    struct {
        uint8_t KeyId;
        uint8_t RndB[CRYPTO_DES_KEY_SIZE];
    } Authenticate;
    struct {
        uint8_t NextIndex;
    } GetApplicationIds;
} DesfireSavedCommandStateType;

typedef enum {
    DESFIRE_IDLE,
    DESFIRE_GET_VERSION2,
    DESFIRE_GET_VERSION3,
    DESFIRE_GET_APPLICATION_IDS2,
    DESFIRE_AUTHENTICATE2,
    DESFIRE_READ_DATA_FILE,
    DESFIRE_WRITE_DATA_FILE,
} DesfireStateType;

static DesfireStateType DesfireState;
static uint8_t AuthenticatedWithKey;
static DesfireSavedCommandStateType DesfireCommandState;

/*
 *
 * The following section implements:
 * DESFire EV0 / D40 specific commands
 *
 */

/*
 * DESFire general commands
 */

static uint16_t EV0CmdGetVersion1(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = DESFIRE_MANUFACTURER_ID;
    Buffer[2] = DESFIRE_TYPE;
    Buffer[3] = DESFIRE_SUBTYPE;
    GetPiccHardwareVersionInfo(&Buffer[4]);
    Buffer[7] = DESFIRE_HW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION2;
    return 8;
}

static uint16_t EV0CmdGetVersion2(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = DESFIRE_MANUFACTURER_ID;
    Buffer[2] = DESFIRE_TYPE;
    Buffer[3] = DESFIRE_SUBTYPE;
    GetPiccSoftwareVersionInfo(&Buffer[4]);
    Buffer[7] = DESFIRE_SW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION3;
    return 8;
}

static uint16_t EV0CmdGetVersion3(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_OPERATION_OK;
    GetPiccManufactureInfo(&Buffer[1]);
    DesfireState = DESFIRE_IDLE;
    return 15;
}

static uint16_t EV0CmdFormatPicc(uint8_t* Buffer, uint16_t ByteCount)
{
    /* Require the PICC app to be selected */
    if (!IsPiccAppSelected()) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify authentication settings */
    if (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is always required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    FormatPicc();
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire key management commands
 */

static uint16_t EV0CmdAuthenticate2KTDEA1(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t KeyId;
    Desfire2KTDEAKeyType Key;

    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    KeyId = Buffer[1];
    /* Validate number of keys: less than max */
    if (KeyId >= GetSelectedAppKeyCount()) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Reset authentication state right away */
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    /* Fetch the key */
    DesfireCommandState.Authenticate.KeyId = KeyId;
    ReadSelectedAppKey(KeyId, Key);
    LogEntry(LOG_APP_AUTH_KEY, Key, sizeof(Key));
    /* Generate the nonce B */
#if 0
    RandomGetBuffer(DesfireCommandState.Authenticate.RndB, DESFIRE_2KTDEA_NONCE_SIZE);
#else
    /* Fixed nonce for testing */
    DesfireCommandState.Authenticate.RndB[0] = 0xCA;
    DesfireCommandState.Authenticate.RndB[1] = 0xFE;
    DesfireCommandState.Authenticate.RndB[2] = 0xBA;
    DesfireCommandState.Authenticate.RndB[3] = 0xBE;
    DesfireCommandState.Authenticate.RndB[4] = 0x00;
    DesfireCommandState.Authenticate.RndB[5] = 0x11;
    DesfireCommandState.Authenticate.RndB[6] = 0x22;
    DesfireCommandState.Authenticate.RndB[7] = 0x33;
#endif
    LogEntry(LOG_APP_NONCE_B, DesfireCommandState.Authenticate.RndB, DESFIRE_2KTDEA_NONCE_SIZE);
    /* Encipher the nonce B with the selected key; <= 8 bytes = no CBC */
    CryptoEncrypt2KTDEA(DesfireCommandState.Authenticate.RndB, &Buffer[1], Key);
    /* Scrub the key */
    memset(&Key, 0, sizeof(Key));

    /* Done */
    DesfireState = DESFIRE_AUTHENTICATE2;
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    return DESFIRE_STATUS_RESPONSE_SIZE + DESFIRE_2KTDEA_NONCE_SIZE;
}

static uint16_t EV0CmdAuthenticate2KTDEA2(uint8_t* Buffer, uint16_t ByteCount)
{
    Desfire2KTDEAKeyType Key;

    /* Validate command length */
    if (ByteCount != 1 + 2 * CRYPTO_DES_BLOCK_SIZE) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Fetch the key */
    ReadSelectedAppKey(DesfireCommandState.Authenticate.KeyId, Key);
    LogEntry(LOG_APP_AUTH_KEY, Key, sizeof(Key));
    /* Encipher to obtain plain text; zero IV = no CBC */
    memset(SessionIV, 0, sizeof(SessionIV));
    CryptoEncrypt2KTDEA_CBCReceive(2, &Buffer[1], &Buffer[1], SessionIV, Key);
    LogEntry(LOG_APP_NONCE_AB, &Buffer[1], 2 * DESFIRE_2KTDEA_NONCE_SIZE);
    /* Now, RndA is at Buffer[1], RndB' is at Buffer[9] */
    if (memcmp(&Buffer[9], &DesfireCommandState.Authenticate.RndB[1], DESFIRE_2KTDEA_NONCE_SIZE - 1) || (Buffer[16] != DesfireCommandState.Authenticate.RndB[0])) {
        /* Scrub the key */
        memset(&Key, 0, sizeof(Key));
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Compose the session key */
    SessionKey[0] = Buffer[1];
    SessionKey[1] = Buffer[2];
    SessionKey[2] = Buffer[3];
    SessionKey[3] = Buffer[4];
    SessionKey[4] = DesfireCommandState.Authenticate.RndB[0];
    SessionKey[5] = DesfireCommandState.Authenticate.RndB[1];
    SessionKey[6] = DesfireCommandState.Authenticate.RndB[2];
    SessionKey[7] = DesfireCommandState.Authenticate.RndB[3];
    SessionKey[8] = Buffer[5];
    SessionKey[9] = Buffer[6];
    SessionKey[10] = Buffer[7];
    SessionKey[11] = Buffer[8];
    SessionKey[12] = DesfireCommandState.Authenticate.RndB[4];
    SessionKey[13] = DesfireCommandState.Authenticate.RndB[5];
    SessionKey[14] = DesfireCommandState.Authenticate.RndB[6];
    SessionKey[15] = DesfireCommandState.Authenticate.RndB[7];
    AuthenticatedWithKey = DesfireCommandState.Authenticate.KeyId;
    /* Rotate the nonce A left by 8 bits */
    Buffer[9] = Buffer[1];
    /* Encipher the nonce A; <= 8 bytes = no CBC */
    CryptoEncrypt2KTDEA(&Buffer[2], &Buffer[1], Key);
    /* Scrub the key */
    memset(&Key, 0, sizeof(Key));
    /* NOTE: EV0: The session IV is reset on each transfer for legacy authentication */

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + DESFIRE_2KTDEA_NONCE_SIZE;
}

static uint16_t EV0CmdChangeKey(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t KeyId;
    uint8_t ChangeKeyId;
    uint8_t KeySettings;

    /* Validate command length */
    if (ByteCount != 1 + 1 + 3 * CRYPTO_DES_BLOCK_SIZE) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    KeyId = Buffer[1];
    /* Validate number of keys: less than max */
    if (KeyId >= GetSelectedAppKeyCount()) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate the state against change key settings */
    KeySettings = GetSelectedAppKeySettings();
    ChangeKeyId = KeySettings >> 4;
    switch (ChangeKeyId) {
    case DESFIRE_ALL_KEYS_FROZEN:
        /* Only master key may be (potentially) changed */
        if (KeyId != DESFIRE_MASTER_KEY_ID || !(KeySettings & DESFIRE_ALLOW_MASTER_KEY_CHANGE)) {
            Buffer[0] = STATUS_PERMISSION_DENIED;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
        break;
    case DESFIRE_USE_TARGET_KEY:
        /* Authentication with the target key is required */
        if (KeyId != AuthenticatedWithKey) {
            Buffer[0] = STATUS_PERMISSION_DENIED;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
        break;
    default:
        /* Authentication with a specific key is required */
        if (KeyId != ChangeKeyId) {
            Buffer[0] = STATUS_PERMISSION_DENIED;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
        break;
    }

    /* Encipher to obtain plaintext */
    memset(SessionIV, 0, sizeof(SessionIV));
    CryptoEncrypt2KTDEA_CBCReceive(3, &Buffer[2], &Buffer[2], SessionIV, SessionKey);
    /* Verify the checksum first */
    if (!ISO14443ACheckCRCA(&Buffer[2], sizeof(Desfire2KTDEAKeyType))) {
        Buffer[0] = STATUS_INTEGRITY_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* The PCD generates data differently based on whether AuthKeyId == ChangeKeyId */
    if (KeyId != AuthenticatedWithKey) {
        Desfire2KTDEAKeyType OldKey;
        uint8_t i;

        /* NewKey^OldKey | CRC(NewKey^OldKey) | CRC(NewKey) | Padding */
        ReadSelectedAppKey(KeyId, OldKey);
        for (i = 0; i < sizeof(OldKey); ++i) {
            Buffer[2 + i] ^= OldKey[i];
            OldKey[i] = 0;
        }
        /* Overwrite the old CRC */
        Buffer[2 + 16] = Buffer[2 + 18];
        Buffer[2 + 17] = Buffer[2 + 19];
        /* Verify the checksum again */
        if (!ISO14443ACheckCRCA(&Buffer[2], sizeof(Desfire2KTDEAKeyType))) {
            Buffer[0] = STATUS_INTEGRITY_ERROR;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
    }
    else {
        /* NewKey | CRC(NewKey) | Padding */
        AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    }
    /* NOTE: Padding checks are skipped, because meh. */

    /* Write the key and scrub */
    WriteSelectedAppKey(KeyId, &Buffer[2]);
    memset(&Buffer[2], 0, sizeof(Desfire2KTDEAKeyType));

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdGetKeySettings(uint8_t* Buffer, uint16_t ByteCount)
{
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    Buffer[1] = GetSelectedAppKeySettings();
    Buffer[2] = GetSelectedAppKeyCount();

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2;
}

static uint16_t EV0CmdChangeKeySettings(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t NewSettings;

    /* Validate command length */
    if (ByteCount != 1 + CRYPTO_DES_BLOCK_SIZE) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify whether settings are changeable */
    if (!(GetSelectedAppKeySettings() & DESFIRE_ALLOW_CONFIG_CHANGE)) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify the master key has been authenticated with */
    if (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Encipher to obtain plaintext */
    CryptoEncrypt2KTDEA(&Buffer[2], &Buffer[2], SessionKey);
    /* Verify the checksum first */
    if (!ISO14443ACheckCRCA(&Buffer[2], sizeof(Desfire2KTDEAKeyType))) {
        Buffer[0] = STATUS_INTEGRITY_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    NewSettings = Buffer[1];
    if (IsPiccAppSelected()) {
        NewSettings &= 0x0F;
    }
    SetSelectedAppKeySettings(NewSettings);

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2;
}

/*
 * DESFire application management commands
 */

static uint16_t GetApplicationIdsIterator(uint8_t* Buffer, uint16_t ByteCount)
{
    TransferStatus Status;

    Status = GetApplicationIdsTransfer(&Buffer[1]);
    if (Status.IsComplete) {
        Buffer[0] = STATUS_OPERATION_OK;
        DesfireState = DESFIRE_IDLE;
    }
    else {
        Buffer[0] = STATUS_ADDITIONAL_FRAME;
        DesfireState = DESFIRE_GET_APPLICATION_IDS2;
    }
    return DESFIRE_STATUS_RESPONSE_SIZE + Status.BytesProcessed;
}

static uint16_t EV0CmdGetApplicationIds1(uint8_t* Buffer, uint16_t ByteCount)
{
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Require the PICC app to be selected */
    if (!IsPiccAppSelected()) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify authentication settings */
    if (!(GetSelectedAppKeySettings() & DESFIRE_FREE_DIRECTORY_LIST) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Setup the job and jump to the worker routine */
    GetApplicationIdsSetup();
    return GetApplicationIdsIterator(Buffer, ByteCount);
}

static uint16_t EV0CmdCreateApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    const DesfireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };
    uint8_t KeyCount;
    uint8_t KeySettings;

    /* Require the PICC app to be selected */
    if (!IsPiccAppSelected()) {
        Status = STATUS_PERMISSION_DENIED;
        goto exit_with_status;
    }
    /* Validate command length */
    if (ByteCount != 1 + 3 + 1 + 1) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    /* Verify authentication settings */
    if (!(GetSelectedAppKeySettings() & DESFIRE_FREE_CREATE_DELETE) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is required */
        Status = STATUS_AUTHENTICATION_ERROR;
        goto exit_with_status;
    }
    /* Validate number of keys: less than max */
    KeyCount = Buffer[5];
    if (KeyCount > DESFIRE_MAX_KEYS) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    KeySettings = Buffer[4];
    /* Done */
    Status = CreateApp(Aid, KeyCount, KeySettings);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdDeleteApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    const DesfireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };
    uint8_t PiccKeySettings;

    /* Validate command length */
    if (ByteCount != 1 + 3) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    /* Validate AID: AID of all zeros cannot be deleted */
    if ((Aid[0] | Aid[1] | Aid[2]) == 0x00) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    /* Validate authentication: a master key is always required */
    if (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        Status = STATUS_AUTHENTICATION_ERROR;
        goto exit_with_status;
    }
    /* Validate authentication: deletion with PICC master key is always OK, but if another app is selected... */
    if (!IsPiccAppSelected()) {
        /* TODO: verify the selected application is the one being deleted */

        PiccKeySettings = GetPiccKeySettings();
        /* Check the PICC key settings whether it is OK to delete using app master key */
        if (!(PiccKeySettings & DESFIRE_FREE_CREATE_DELETE)) {
            Status = STATUS_AUTHENTICATION_ERROR;
            goto exit_with_status;
        }
        SelectPiccApp();
        AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    }

    /* Done */
    Status = DeleteApp(Aid);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdSelectApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    const DesfireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };

    /* Validate command length */
    if (ByteCount != 1 + 3) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Done */
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    Buffer[0] = SelectApp(Aid);
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire application file management commands
 */

static uint8_t CreateFileCommonValidation(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights)
{
    /* Validate file number */
    if (FileNum >= DESFIRE_MAX_FILES) {
        return STATUS_PARAMETER_ERROR;
    }
    /* Validate communication settings */
    /* Validate the access rights */
    return STATUS_OPERATION_OK;
}

static uint16_t EV0CmdCreateStandardDataFile(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    __uint24 FileSize;

    /* Validate command length */
    if (ByteCount != 1 + 1 + 1 + 2 + 3) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    /* Common args validation */
    FileNum = Buffer[1];
    CommSettings = Buffer[2];
    AccessRights = Buffer[3] | (Buffer[4] << 8);
    Status = CreateFileCommonValidation(FileNum, CommSettings, AccessRights);
    if (Status != STATUS_OPERATION_OK) {
        goto exit_with_status;
    }
    /* Validate the file size */
    FileSize = GET_LE24(&Buffer[5]);
    if (FileSize > 8160) {
        Status = STATUS_OUT_OF_EEPROM_ERROR;
        goto exit_with_status;
    }

    Status = CreateStandardFile(FileNum, CommSettings, AccessRights, (uint16_t)FileSize);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdCreateBackupDataFile(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    __uint24 FileSize;

    /* Validate command length */
    if (ByteCount != 1 + 1 + 1 + 2 + 3) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    /* Common args validation */
    FileNum = Buffer[1];
    CommSettings = Buffer[2];
    AccessRights = Buffer[3] | (Buffer[4] << 8);
    Status = CreateFileCommonValidation(FileNum, CommSettings, AccessRights);
    if (Status != STATUS_OPERATION_OK) {
        goto exit_with_status;
    }
    /* Validate the file size */
    FileSize = GET_LE24(&Buffer[5]);
    if (FileSize > 4096) {
        Status = STATUS_OUT_OF_EEPROM_ERROR;
        goto exit_with_status;
    }

    Status = CreateBackupFile(FileNum, CommSettings, AccessRights, (uint16_t)FileSize);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdCreateValueFile(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdCreateLinearRecordFile(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdCreateCyclicRecordFile(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdDeleteFile(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    uint8_t FileNum;

    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    FileNum = Buffer[1];
    /* Validate file number */
    if (FileNum >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    /* Validate access settings */
    if (!(GetSelectedAppKeySettings() & DESFIRE_FREE_CREATE_DELETE) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        Status = STATUS_AUTHENTICATION_ERROR;
        goto exit_with_status;
    }

    Status = DeleteFile(FileNum);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdGetFileIds(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdGetFileSettings(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdChangeFileSettings(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire data manipulation commands
 */

#define VALIDATE_ACCESS_READWRITE   (1 << 0)
#define VALIDATE_ACCESS_WRITE       (1 << 1)
#define VALIDATE_ACCESS_READ        (1 << 2)

#define VALIDATED_ACCESS_DENIED 0
#define VALIDATED_ACCESS_GRANTED 1
#define VALIDATED_ACCESS_GRANTED_PLAINTEXT 2

static uint8_t ValidateAuthentication(uint16_t AccessRights, uint8_t CheckMask)
{
    uint8_t RequiredKeyId;
    bool HaveFreeAccess = false;

    AccessRights >>= 4;
    while (CheckMask) {
        if (CheckMask & 1) {
            RequiredKeyId = AccessRights & 0x0F;
            if (AuthenticatedWithKey == RequiredKeyId) {
                return VALIDATED_ACCESS_GRANTED;
            }
            if (RequiredKeyId == DESFIRE_ACCESS_FREE) {
                HaveFreeAccess = true;
            }
        }
        CheckMask >>= 1;
        AccessRights >>= 4;
    }
    return HaveFreeAccess ? VALIDATED_ACCESS_GRANTED_PLAINTEXT : VALIDATED_ACCESS_DENIED;
}

static uint16_t ReadDataFileIterator(uint8_t* Buffer, uint16_t ByteCount)
{
    TransferStatus Status;
    /* NOTE: incoming ByteCount is not verified here for now */

    Status = ReadDataFileTransfer(&Buffer[1]);
    if (Status.IsComplete) {
        Buffer[0] = STATUS_OPERATION_OK;
        DesfireState = DESFIRE_IDLE;
    }
    else {
        Buffer[0] = STATUS_ADDITIONAL_FRAME;
        DesfireState = DESFIRE_READ_DATA_FILE;
    }
    return DESFIRE_STATUS_RESPONSE_SIZE + Status.BytesProcessed;
}

static uint16_t EV0CmdReadData(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    __uint24 Offset;
    __uint24 Length;

    /* Validate command length */
    if (ByteCount != 1 + 1 + 3 + 3) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    FileNum = Buffer[1];
    /* Validate file number */
    if (FileNum >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }

    Status = SelectFile(FileNum);
    if (Status != STATUS_OPERATION_OK) {
        goto exit_with_status;
    }

    CommSettings = GetSelectedFileCommSettings();
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(GetSelectedFileAccessRights(), VALIDATE_ACCESS_READWRITE|VALIDATE_ACCESS_READ)) {
    case VALIDATED_ACCESS_DENIED:
        Status = STATUS_AUTHENTICATION_ERROR;
        goto exit_with_status;
    case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
        CommSettings = DESFIRE_COMMS_PLAINTEXT;
        /* Fall through */
    case VALIDATED_ACCESS_GRANTED:
        /* Carry on */
        break;
    }

    /* Validate the file type */
    if (GetSelectedFileType() != DESFIRE_FILE_STANDARD_DATA && GetSelectedFileType() != DESFIRE_FILE_BACKUP_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    /* Validate offset and length (preliminary) */
    Offset = GET_LE24(&Buffer[2]);
    Length = GET_LE24(&Buffer[5]);
    if (Offset > 8192 || Length > 8192) {
        Status = STATUS_BOUNDARY_ERROR;
        goto exit_with_status;
    }

    /* Setup and start the transfer */
    Status = ReadDataFileSetup(CommSettings, (uint16_t)Offset, (uint16_t)Length);
    if (Status) {
        goto exit_with_status;
    }
    return ReadDataFileIterator(Buffer, 1);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint8_t WriteDataFileInternal(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;

    Status = WriteDataFileTransfer(Buffer, ByteCount);

    switch (Status) {
    default:
        /* In case anything goes wrong, abort things */
        AbortTransaction();
        /* Fall through */
    case STATUS_OPERATION_OK:
        DesfireState = DESFIRE_IDLE;
        break;
    case STATUS_ADDITIONAL_FRAME:
        DesfireState = DESFIRE_WRITE_DATA_FILE;
        break;
    }
    return Status;
}

static uint16_t WriteDataFileIterator(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = WriteDataFileInternal(&Buffer[1], ByteCount - 1);
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdWriteData(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    __uint24 Offset;
    __uint24 Length;

    /* Validate command length */
    if (ByteCount < 1 + 1 + 3 + 3) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    FileNum = Buffer[1];
    /* Validate file number */
    if (FileNum >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }

    Status = SelectFile(FileNum);
    if (Status != STATUS_OPERATION_OK) {
        goto exit_with_status;
    }

    CommSettings = GetSelectedFileCommSettings();
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(GetSelectedFileAccessRights(), VALIDATE_ACCESS_READWRITE|VALIDATE_ACCESS_WRITE)) {
    case VALIDATED_ACCESS_DENIED:
        Status = STATUS_AUTHENTICATION_ERROR;
        goto exit_with_status;
    case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
        CommSettings = DESFIRE_COMMS_PLAINTEXT;
        /* Fall through */
    case VALIDATED_ACCESS_GRANTED:
        /* Carry on */
        break;
    }

    /* Validate the file type */
    if (GetSelectedFileType() != DESFIRE_FILE_STANDARD_DATA && GetSelectedFileType() != DESFIRE_FILE_BACKUP_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    /* Validate offset and length (preliminary) */
    Offset = GET_LE24(&Buffer[2]);
    Length = GET_LE24(&Buffer[5]);
    if (Offset > 8192 || Length > 8192) {
        Status = STATUS_BOUNDARY_ERROR;
        goto exit_with_status;
    }

    /* Setup and start the transfer */
    Status = WriteDataFileSetup(CommSettings, (uint16_t)Offset, (uint16_t)Length);
    if (Status) {
        goto exit_with_status;
    }
    Status = WriteDataFileIterator(&Buffer[8], ByteCount - 8);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdGetValue(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    TransferStatus XferStatus;

    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    FileNum = Buffer[1];
    /* Validate file number */
    if (FileNum >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }

    Status = SelectFile(FileNum);
    if (Status != STATUS_OPERATION_OK) {
        goto exit_with_status;
    }

    CommSettings = GetSelectedFileCommSettings();
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(GetSelectedFileAccessRights(), VALIDATE_ACCESS_READWRITE|VALIDATE_ACCESS_READ|VALIDATE_ACCESS_WRITE)) {
    case VALIDATED_ACCESS_DENIED:
        Status = STATUS_AUTHENTICATION_ERROR;
        goto exit_with_status;
    case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
        CommSettings = DESFIRE_COMMS_PLAINTEXT;
        /* Fall through */
    case VALIDATED_ACCESS_GRANTED:
        /* Carry on */
        break;
    }

    /* Validate the file type */
    if (GetSelectedFileType() != DESFIRE_FILE_VALUE_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }

    /* Setup and start the transfer */
    Status = ReadValueFileSetup(CommSettings);
    if (Status) {
        goto exit_with_status;
    }
    XferStatus = ReadDataFileTransfer(&Buffer[1]);
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + XferStatus.BytesProcessed;

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdCredit(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdDebit(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdLimitedCredit(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdReadRecords(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdWriteRecord(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdClearRecords(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire transaction handling commands
 */

static uint16_t EV0CmdCommitTransaction(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdAbortTransaction(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/* Dispatching routines */

static uint16_t MifareDesfireProcessIdle(uint8_t* Buffer, uint16_t ByteCount)
{
    /* Handle EV0 commands */
    switch (Buffer[0]) {
    case CMD_GET_VERSION:
        return EV0CmdGetVersion1(Buffer, ByteCount);
    case CMD_FORMAT_PICC:
        return EV0CmdFormatPicc(Buffer, ByteCount);

    case CMD_AUTHENTICATE:
        return EV0CmdAuthenticate2KTDEA1(Buffer, ByteCount);
    case CMD_CHANGE_KEY:
        return EV0CmdChangeKey(Buffer, ByteCount);
    case CMD_GET_KEY_SETTINGS:
        return EV0CmdGetKeySettings(Buffer, ByteCount);
    case CMD_CHANGE_KEY_SETTINGS:
        return EV0CmdChangeKeySettings(Buffer, ByteCount);

    case CMD_GET_APPLICATION_IDS:
        return EV0CmdGetApplicationIds1(Buffer, ByteCount);
    case CMD_CREATE_APPLICATION:
        return EV0CmdCreateApplication(Buffer, ByteCount);
    case CMD_DELETE_APPLICATION:
        return EV0CmdDeleteApplication(Buffer, ByteCount);
    case CMD_SELECT_APPLICATION:
        return EV0CmdSelectApplication(Buffer, ByteCount);

    case CMD_CREATE_STANDARD_DATA_FILE:
        return EV0CmdCreateStandardDataFile(Buffer, ByteCount);
    case CMD_CREATE_BACKUP_DATA_FILE:
        return EV0CmdCreateBackupDataFile(Buffer, ByteCount);
    case CMD_CREATE_VALUE_FILE:
        return EV0CmdCreateValueFile(Buffer, ByteCount);
    case CMD_CREATE_LINEAR_RECORD_FILE:
        return EV0CmdCreateLinearRecordFile(Buffer, ByteCount);
    case CMD_CREATE_CYCLIC_RECORD_FILE:
        return EV0CmdCreateCyclicRecordFile(Buffer, ByteCount);
    case CMD_DELETE_FILE:
        return EV0CmdDeleteFile(Buffer, ByteCount);
    case CMD_GET_FILE_IDS:
        return EV0CmdGetFileIds(Buffer, ByteCount);
    case CMD_GET_FILE_SETTINGS:
        return EV0CmdGetFileSettings(Buffer, ByteCount);
    case CMD_CHANGE_FILE_SETTINGS:
        return EV0CmdChangeFileSettings(Buffer, ByteCount);

    case CMD_READ_DATA:
        return EV0CmdReadData(Buffer, ByteCount);
    case CMD_WRITE_DATA:
        return EV0CmdWriteData(Buffer, ByteCount);

    case CMD_GET_VALUE:
        return EV0CmdGetValue(Buffer, ByteCount);
    case CMD_CREDIT:
        return EV0CmdCredit(Buffer, ByteCount);
    case CMD_DEBIT:
        return EV0CmdDebit(Buffer, ByteCount);
    case CMD_LIMITED_CREDIT:
        return EV0CmdLimitedCredit(Buffer, ByteCount);

    case CMD_READ_RECORDS:
        return EV0CmdReadRecords(Buffer, ByteCount);
    case CMD_WRITE_RECORD:
        return EV0CmdWriteRecord(Buffer, ByteCount);
    case CMD_CLEAR_RECORD_FILE:
        return EV0CmdClearRecords(Buffer, ByteCount);

    case CMD_COMMIT_TRANSACTION:
        return EV0CmdCommitTransaction(Buffer, ByteCount);
    case CMD_ABORT_TRANSACTION:
        return EV0CmdAbortTransaction(Buffer, ByteCount);

    default:
        break;
    }

    /* Handle EV1 commands, if enabled */
    if (IsEmulatingEV1()) {
        /* To be implemented */
    }

    /* Handle EV2 commands -- in future */

    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t MifareDesfireProcessCommand(uint8_t* Buffer, uint16_t ByteCount)
{
    if (DesfireState == DESFIRE_IDLE)
        return MifareDesfireProcessIdle(Buffer, ByteCount);

    /* Expecting further data here */
    if (Buffer[0] != STATUS_ADDITIONAL_FRAME) {
        AbortTransaction();
        DesfireState = DESFIRE_IDLE;
        Buffer[0] = STATUS_COMMAND_ABORTED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    switch (DesfireState) {
    case DESFIRE_GET_VERSION2:
        return EV0CmdGetVersion2(Buffer, ByteCount);
    case DESFIRE_GET_VERSION3:
        return EV0CmdGetVersion3(Buffer, ByteCount);
    case DESFIRE_GET_APPLICATION_IDS2:
        return GetApplicationIdsIterator(Buffer, ByteCount);
    case DESFIRE_AUTHENTICATE2:
        return EV0CmdAuthenticate2KTDEA2(Buffer, ByteCount);
    case DESFIRE_READ_DATA_FILE:
        return ReadDataFileIterator(Buffer, ByteCount);
    default:
        /* Should not happen. */
        Buffer[0] = STATUS_PICC_INTEGRITY_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
}

static uint16_t MifareDesfireProcess(uint8_t* Buffer, uint16_t ByteCount)
{
    /* TODO: Properly detect ISO 7816-4 PDUs and switch modes to avoid doing ths all the time */
    if (Buffer[0] == 0x90 && Buffer[2] == 0x00 && Buffer[3] == 0x00 && Buffer[4] == ByteCount - 6) {
        /* Unwrap the PDU from ISO 7816-4 */
        ByteCount = Buffer[4];
        Buffer[0] = Buffer[1];
        memmove(&Buffer[1], &Buffer[5], ByteCount);
        /* Process the command */
        ByteCount = MifareDesfireProcessCommand(Buffer, ByteCount + 1);
        if (ByteCount) {
            /* Re-wrap into ISO 7816-4 */
            Buffer[ByteCount] = Buffer[0];
            memmove(&Buffer[0], &Buffer[1], ByteCount - 1);
            Buffer[ByteCount - 1] = 0x91;
            ByteCount++;
        }
        return ByteCount;
    }
    else {
        /* ISO/IEC 14443-4 PDUs, no extra work */
        return MifareDesfireProcessCommand(Buffer, ByteCount);
    }
}

static void MifareDesfireReset(void)
{
    ResetPiccBackend();
    DesfireState = DESFIRE_IDLE;
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
}

/*
 * ISO/IEC 14443-4 implementation
 * Currently this only supports wrapping things in 14443-4 I-blocks.
 * To support EV2 cards emulation, proper support for handling 14443-4 
 * blocks will be implemented.
 */

void ISO144433AHalt(void);

typedef enum {
    ISO14443_4_STATE_EXPECT_RATS,
    ISO14443_4_STATE_ACTIVE,
} Iso144434StateType;

static Iso144434StateType Iso144434State;
static uint8_t Iso144434BlockNumber;
static uint8_t Iso144434CardID;

static void ISO144434SwitchState(Iso144434StateType NewState)
{
    Iso144434State = NewState;
    LogEntry(LOG_ISO14443_4_STATE, &Iso144434State, 1);
}

void ISO144434Reset(void)
{
    /* No logging -- spams the log */
    Iso144434State = ISO14443_4_STATE_EXPECT_RATS;
    Iso144434BlockNumber = 1;
}

uint16_t ISO144434ProcessBlock(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t PCB = Buffer[0];
    uint8_t MyBlockNumber = Iso144434BlockNumber;
    uint8_t PrologueLength;
    uint8_t HaveCID, HaveNAD;

    /* Verify the block's length: at the very least PCB + CRCA */
    if (ByteCount < (1 + ISO14443A_CRCA_SIZE)) {
        /* Huh? Broken frame? */
        /* TODO: LOG ME */
        DEBUG_PRINT("\r\nISO14443-4: length fail");
        return ISO14443A_APP_NO_RESPONSE;
    }
    ByteCount -= 2;

    /* Verify the checksum; fail if doesn't match */
    if (!ISO14443ACheckCRCA(Buffer, ByteCount)) {
        LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, ByteCount);
        /* ISO/IEC 14443-4, clause 7.5.5. The PICC does not attempt any error recovery. */
        DEBUG_PRINT("\r\nISO14443-4: CRC fail; %04X vs %04X", *(uint16_t*)&Buffer[ByteCount], ISO14443AComputeCRCA(Buffer, ByteCount));
        return ISO14443A_APP_NO_RESPONSE;
    }

    switch (Iso144434State) {
    case ISO14443_4_STATE_EXPECT_RATS:
        /* See: ISO/IEC 14443-4, clause 5.6.1.2 */
        if (Buffer[0] != ISO14443A_CMD_RATS) {
            /* Ignore blocks other than RATS and HLTA */
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Process RATS.
         * NOTE: ATS bytes are tailored to Chameleon implementation and differ from DESFire spec.
         * NOTE: Some PCD implementations do a memcmp() over ATS bytes, which is completely wrong.
         */
        Iso144434CardID = Buffer[1] & 0x0F;

        Buffer[0] = DESFIRE_EV0_ATS_TL_BYTE;
        Buffer[1] = DESFIRE_EV0_ATS_T0_BYTE;
        Buffer[2] = DESFIRE_EV0_ATS_TA_BYTE;
        Buffer[3] = DESFIRE_EV0_ATS_TB_BYTE;
        Buffer[4] = DESFIRE_EV0_ATS_TC_BYTE;
        Buffer[5] = 0x80; /* T1: dummy value for historical bytes */

        ISO144434SwitchState(ISO14443_4_STATE_ACTIVE);
        ByteCount = Buffer[0];
        break;

    case ISO14443_4_STATE_ACTIVE:
        /* See: ISO/IEC 14443-4; 7.1 Block format */

        /* Parse the prologue */
        PrologueLength = 1;
        HaveCID = PCB & ISO14443_PCB_HAS_CID_MASK;
        if (HaveCID) {
            PrologueLength++;
            /* Verify the card ID */
            if (Buffer[1] != Iso144434CardID) {
                /* Different card ID => the frame is ignored */
                return ISO14443A_APP_NO_RESPONSE;
            }
        }

        switch (PCB & ISO14443_PCB_BLOCK_TYPE_MASK) {
        case ISO14443_PCB_I_BLOCK:
            HaveNAD = PCB & ISO14443_PCB_HAS_NAD_MASK;
            if (HaveNAD) {
                PrologueLength++;
                /* Not currently supported => the frame is ignored */
                /* TODO: LOG ME */
                return ISO14443A_APP_NO_RESPONSE;
            }
            /* 7.5.3.2, rule D: toggle on each I-block */
            Iso144434BlockNumber = MyBlockNumber = !MyBlockNumber;
            if (PCB & ISO14443_PCB_I_BLOCK_CHAINING_MASK) {
                /* Currently not supported => the frame is ignored */
                /* TODO: LOG ME */
                return ISO14443A_APP_NO_RESPONSE;
            }

            /* Build the prologue for the response */
            /* NOTE: According to the std, incoming/outgoing prologue lengths are equal at all times */
            PCB = ISO14443_PCB_I_BLOCK_STATIC | MyBlockNumber;
            if (HaveCID) {
                PCB |= ISO14443_PCB_HAS_CID_MASK;
                Buffer[1] = Iso144434CardID;
            }
            Buffer[0] = PCB;
            /* Let the DESFire application code process the input data */
            ByteCount = MifareDesfireProcess(Buffer + PrologueLength, ByteCount - PrologueLength);
            /* Short-circuit in case the app decides not to respond at all */
            if (ByteCount == ISO14443A_APP_NO_RESPONSE) {
                return ISO14443A_APP_NO_RESPONSE;
            }
            ByteCount += PrologueLength;
            break;

        case ISO14443_PCB_R_BLOCK:
            /* R-block handling is not yet supported */
            /* TODO: support at least retransmissions */
            return ISO14443A_APP_NO_RESPONSE;

        case ISO14443_PCB_S_BLOCK:
            if (PCB == ISO14443_PCB_S_DESELECT) {
                /* Reset our state */
                ISO144434Reset();
                LogEntry(LOG_ISO14443_4_STATE, &Iso144434State, 1);
                /* Transition to HALT */
                ISO144433AHalt();
                /* Answer with S(DESELECT) -- just send the copy of the message */
                ByteCount = PrologueLength;
                break;
            }
            return ISO14443A_APP_NO_RESPONSE;
        }
        break;
    }

    ISO14443AAppendCRCA(Buffer, ByteCount);
    return ByteCount + ISO14443A_CRCA_SIZE;
}

/*
 * ISO/IEC 14443-3A implementation
 */

typedef enum {
    /* The card is powered up but not selected */
    ISO14443_3A_STATE_IDLE,
    /* Entered on REQA or WUPA; anticollision is being performed */
    ISO14443_3A_STATE_READY1,
    ISO14443_3A_STATE_READY2,
    /* Entered when the card has been selected */
    ISO14443_3A_STATE_ACTIVE,
    /* Something went wrong or we've received a halt command */
    ISO14443_3A_STATE_HALT,
} Iso144433AStateType;

static Iso144433AStateType Iso144433AState;
static Iso144433AStateType Iso144433AIdleState;

static void ISO144433ASwitchState(Iso144433AStateType NewState)
{
    Iso144433AState = NewState;
    LogEntry(LOG_ISO14443_3A_STATE, &Iso144433AState, 1);
}

void ISO144433AReset(void)
{
    /* No logging -- spams the log */
    Iso144433AState = ISO14443_3A_STATE_IDLE;
    Iso144433AIdleState = ISO14443_3A_STATE_IDLE;
}

void ISO144433AHalt(void)
{
    ISO144433ASwitchState(ISO14443_3A_STATE_HALT);
    Iso144433AIdleState = ISO14443_3A_STATE_HALT;
}

static bool ISO144433AIsHalt(const uint8_t* Buffer, uint16_t BitCount)
{
    return BitCount == ISO14443A_HLTA_FRAME_SIZE + ISO14443A_CRCA_SIZE * 8
        && Buffer[0] == ISO14443A_CMD_HLTA
        && Buffer[1] == 0x00
        && ISO14443ACheckCRCA(Buffer, ISO14443A_HLTA_FRAME_SIZE / 8);
}

uint16_t ISO144433APiccProcess(uint8_t* Buffer, uint16_t BitCount)
{
    uint8_t Cmd = Buffer[0];

    /* This implements ISO 14443-3A state machine */
    /* See: ISO/IEC 14443-3, clause 6.2 */
    switch (Iso144433AState) {
    case ISO14443_3A_STATE_HALT:
        if (Cmd != ISO14443A_CMD_WUPA) {
            break;
        }
        /* Fall-through */

    case ISO14443_3A_STATE_IDLE:
        if (Cmd != ISO14443A_CMD_REQA && Cmd != ISO14443A_CMD_WUPA) {
            break;
        }

        Iso144433AIdleState = Iso144433AState;
        ISO144433ASwitchState(ISO14443_3A_STATE_READY1);
        Buffer[0] = (ATQA_VALUE >> 0) & 0x00FF;
        Buffer[1] = (ATQA_VALUE >> 8) & 0x00FF;
        return ISO14443A_ATQA_FRAME_SIZE;

    case ISO14443_3A_STATE_READY1:
        if (Cmd == ISO14443A_CMD_SELECT_CL1) {
            /* Load UID CL1 and perform anticollision. */
            ConfigurationUidType Uid;

            ApplicationGetUid(Uid);
            if (ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
                Uid[3] = Uid[2];
                Uid[2] = Uid[1];
                Uid[1] = Uid[0];
                Uid[0] = ISO14443A_UID0_CT;
            }
            if (ISO14443ASelect(Buffer, &BitCount, Uid, SAK_CL1_VALUE)) {
                /* CL1 stage has ended successfully */
                ISO144433ASwitchState(ISO14443_3A_STATE_READY2);
            }

            return BitCount;
        }
        break;

    case ISO14443_3A_STATE_READY2:
        if (Cmd == ISO14443A_CMD_SELECT_CL2 && ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
            /* Load UID CL2 and perform anticollision */
            ConfigurationUidType Uid;

            ApplicationGetUid(Uid);
            if (ISO14443ASelect(Buffer, &BitCount, &Uid[3], SAK_CL2_VALUE)) {
                /* CL2 stage has ended successfully. This means
                 * our complete UID has been sent to the reader. */
                ISO144433ASwitchState(ISO14443_3A_STATE_ACTIVE);
            }

            return BitCount;
        }
        break;

    case ISO14443_3A_STATE_ACTIVE:
        /* Recognise the HLTA command */
        if (ISO144433AIsHalt(Buffer, BitCount)) {
            LogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
            ISO144433ASwitchState(ISO14443_3A_STATE_HALT);
            DEBUG_PRINT("\r\nISO14443-3: Got HALT");
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Forward to ISO/IEC 14443-4 processing code */
        return ISO144434ProcessBlock(Buffer, (BitCount + 7) / 8) * 8;
    }

    /* Unknown command. Reset back to idle/halt state. */
    DEBUG_PRINT("\r\nISO14443-3: RESET TO IDLE");
    ISO144433ASwitchState(Iso144433AIdleState);
    return ISO14443A_APP_NO_RESPONSE;
}

extern void RunTDEATests(void);

void MifareDesfireEV0AppInit(void)
{
    /* Init lower layers: nothing for now */
    InitialisePiccBackendEV0(DESFIRE_STORAGE_SIZE_4K);
    /* The rest is handled in reset */
}

static void MifareDesfireEV1AppInit(uint8_t StorageSize)
{
    /* Init lower layers: nothing for now */
    InitialisePiccBackendEV1(StorageSize);
    /* The rest is handled in reset */
}

void MifareDesfire2kEV1AppInit(void)
{
    MifareDesfireEV1AppInit(DESFIRE_STORAGE_SIZE_2K);
}

void MifareDesfire4kEV1AppInit(void)
{
    MifareDesfireEV1AppInit(DESFIRE_STORAGE_SIZE_4K);
}

void MifareDesfire8kEV1AppInit(void)
{
    MifareDesfireEV1AppInit(DESFIRE_STORAGE_SIZE_8K);
}

void MifareDesfireAppReset(void)
{
    /* This is called repeatedly, so limit the amount of work done */
    ISO144433AReset();
    ISO144434Reset();
    MifareDesfireReset();
}

void MifareDesfireAppTask(void)
{
    /* Empty */
}

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
    return ISO144433APiccProcess(Buffer, BitCount);
}

void MifareDesfireGetUid(ConfigurationUidType Uid)
{
    GetPiccUid(Uid);
}

void MifareDesfireSetUid(ConfigurationUidType Uid)
{
    SetPiccUid(Uid);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
