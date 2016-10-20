/*
 * MifareDesfire.c
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "MifareDesfire.h"

#include "ISO14443-4.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"
#include "../Random.h"

/* Anticollision parameters */
#define ATQA_VALUE              0x0344
#define SAK_CL1_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT | ISO14443A_SAK_INCOMPLETE)
#define SAK_CL2_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT)

/* UID location in the card memory; sizes are in bytes */
#define MEM_UID_CL1_ADDRESS         0x00
#define MEM_UID_CL1_SIZE            3
#define MEM_UID_BCC1_ADDRESS        0x03
#define MEM_UID_CL2_ADDRESS         0x04
#define MEM_UID_CL2_SIZE            4
#define MEM_UID_BCC2_ADDRESS        0x08

#define STATUS_FRAME_SIZE           (1 * 8) /* Bits */
#define ATS_FRAME_SIZE              6 /* Bytes */

#define ATS_TL_BYTE 0x06 /* TL: ATS length, 6 bytes */
#define ATS_T0_BYTE 0x75 /* T0: TA, TB, TC present; max accepted frame is 64 bytes */
#define ATS_TA_BYTE 0x00 /* TA: Only the lowest bit rate is supported */
#define ATS_TB_BYTE 0x81 /* TB: taken from the DESFire spec */
#define ATS_TC_BYTE 0x02 /* TC: taken from the DESFire spec */

/*
 * DESFire-specific things follow
 */

#define ID_PHILIPS_NXP           0x04

/* Defines for GetVersion */
#define DESFIRE_MANUFACTURER_ID         ID_PHILIPS_NXP
#define DESFIRE_TYPE_MF3ICD40           0x01
#define DESFIRE_SUBTYPE_MF3ICD40        0x01
#define DESFIRE_HW_MAJOR_VERSION_NO     0x01
#define DESFIRE_HW_MINOR_VERSION_NO     0x01
#define DESFIRE_HW_PROTOCOL_TYPE        0x05
#define DESFIRE_SW_MAJOR_VERSION_NO     0x01
#define DESFIRE_SW_MINOR_VERSION_NO     0x01
#define DESFIRE_SW_PROTOCOL_TYPE        0x05
#define DESFIRE_STORAGE_SIZE            0x18 /* = 4k */

/* Based on section 3.4 */
typedef enum {
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
} DesfireStatusCodeType;

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

#define DESFIRE_NONCE_SIZE 8 /* Bytes */
/* Authentication status */
#define DESFIRE_MASTER_KEY_ID 0
#define DESFIRE_NOT_AUTHENTICATED 0xFF

typedef enum {
    DESFIRE_IDLE,
    /* Handling GetVersion's multiple frames */
    DESFIRE_GET_VERSION2,
    DESFIRE_GET_VERSION3,
    DESFIRE_GET_APPLICATION_IDS2,
    DESFIRE_AUTHENTICATE2,
} DesfireStateType;

typedef union {
    struct {
        uint8_t KeyId;
        uint8_t RndB[CRYPTO_DES_KEY_SIZE];
    } Authenticate;
    struct {
        MifareDesfireAppDirEntryType *NextEntry;
    } GetApplicationIds;
} DesfireSavedCommandStateType;

static DesfireStateType DesfireState;
static DesfireSavedCommandStateType DesfireCommandState;
static uint8_t AuthenticatedWithKey;
static MifareDesfireKeyType SessionKey;
/* Cached data */
static MifareDesfirePiccInfoType Picc;
static MifareDesfireAppDirType AppDir;
static MifareDesfireAppDirEntryType* SelectedAppEntry;
static uint8_t SelectedAppKeySettings;
static uint8_t SelectedAppKeyCount;
static MifareDesfireKeyType* SelectedAppKeyAddress; /* in FRAM */

static void MifareDesfireSelectPiccApp(void)
{
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    SelectedAppEntry = NULL;
    SelectedAppKeySettings = Picc.MasterKeySettings & 0x0F;
    SelectedAppKeyCount = 1;
    SelectedAppKeyAddress = MIFARE_DESFIRE_PICC_INFO_BLOCK_ID * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE
        + &((MifareDesfirePiccInfoType*)NULL)->MasterKey;
}

/* Return nonzero if AID 00 00 00 is selected */
static uint8_t MifareDesfireIsPiccAppSelected(void)
{
    return SelectedAppEntry == NULL;
}

static MifareDesfireAppDirEntryType* MifareDesfireLookupAppByAid(uint8_t Aid0, uint8_t Aid1, uint8_t Aid2)
{
    uint8_t Slot;
    MifareDesfireAppDirEntryType* Entry;

    for (Slot = 0, Entry = &AppDir.Apps[0]; Slot < MIFARE_DESFIRE_MAX_APPS; ++Slot, ++Entry) {
        if (Entry->Aid[0] == Aid0 && Entry->Aid[1] == Aid1 && Entry->Aid[2] == Aid2)
            return Entry;
    }
    return NULL;
}

static void MifareDesfireSynchronizeAppDir(void)
{
    MemoryWriteBlock(&AppDir, MIFARE_DESFIRE_APP_DIR_BLOCK_ID * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, sizeof(MifareDesfireAppDirType));
}

static void MifareDesfireSynchronizePiccInfo(void)
{
    MemoryWriteBlock(&Picc, MIFARE_DESFIRE_PICC_INFO_BLOCK_ID * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, sizeof(Picc));
}

static uint8_t MifareDesfireEepromAllocate(uint8_t BlockCount)
{
    uint8_t Block;

    /* Check if we have space */
    Block = Picc.FirstFreeBlock;
    if (Block + BlockCount < Block) {
        return 0;
    }
    Picc.FirstFreeBlock = Block + BlockCount;
    MifareDesfireSynchronizePiccInfo();
    return Block;
}

/*
 * DESFire general commands
 */

static uint16_t MifareDesfireCmdGetVersion1(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = DESFIRE_MANUFACTURER_ID;
    Buffer[2] = DESFIRE_TYPE_MF3ICD40;
    Buffer[3] = DESFIRE_SUBTYPE_MF3ICD40;
    Buffer[4] = DESFIRE_HW_MAJOR_VERSION_NO;
    Buffer[5] = DESFIRE_HW_MINOR_VERSION_NO;
    Buffer[6] = DESFIRE_STORAGE_SIZE;
    Buffer[7] = DESFIRE_HW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION2;
    return 8;
}

static uint16_t MifareDesfireCmdGetVersion2(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = DESFIRE_MANUFACTURER_ID;
    Buffer[2] = DESFIRE_TYPE_MF3ICD40;
    Buffer[3] = DESFIRE_SUBTYPE_MF3ICD40;
    Buffer[4] = DESFIRE_SW_MAJOR_VERSION_NO;
    Buffer[5] = DESFIRE_SW_MINOR_VERSION_NO;
    Buffer[6] = DESFIRE_STORAGE_SIZE;
    Buffer[7] = DESFIRE_SW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION3;
    return 8;
}

static uint16_t MifareDesfireCmdGetVersion3(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_OPERATION_OK;
    /* UID */
    ApplicationGetUid(&Buffer[1]);
    /* Batch number */
    Buffer[8] = 0;
    Buffer[9] = 0;
    Buffer[10] = 0;
    Buffer[11] = 0;
    Buffer[12] = 0;
    /* Calendar week and year */
    Buffer[13] = 52;
    Buffer[14] = 16;
    DesfireState = DESFIRE_IDLE;
    return 15;
}

static void MifareDesfireFormatPicc(void)
{
    /* Wipe application directory */
    memset(&AppDir, 0, sizeof(AppDir));
    /* Reset the free block pointer */
    Picc.FirstFreeBlock = MIFARE_DESFIRE_APP_DIR_BLOCK_ID + MIFARE_DESFIRE_APP_DIR_BLOCKS;
    MifareDesfireSynchronizePiccInfo();
    MifareDesfireSynchronizeAppDir();
}

static uint16_t MifareDesfireCmdFormatPicc(uint8_t* Buffer, uint16_t ByteCount)
{
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Require zero AID to be selected */
    if (!MifareDesfireIsPiccAppSelected()) {
        /* TODO: Verify this is the proper error code */
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify authentication settings */
    if (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is always required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    MifareDesfireFormatPicc();
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire key management commands
 */

static uint16_t MifareDesfireCmdAuthenticate1(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t KeyId;
    MifareDesfireKeyType Key;

    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    KeyId = Buffer[1];
    /* Validate number of keys: less than max */
    if (KeyId >= SelectedAppKeyCount) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Reset authentication state right away */
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    /* Fetch the key */
    DesfireCommandState.Authenticate.KeyId = KeyId;
    MemoryReadBlock(&Key, (uint16_t)&SelectedAppKeyAddress[KeyId], sizeof(Key));
    /* Generate the nonce B */
    RandomGetBuffer(DesfireCommandState.Authenticate.RndB, DESFIRE_NONCE_SIZE);
    /* Encipher the nonce B with the selected key */
    CryptoEncrypt_3DES_KeyOption2_CBC(DESFIRE_NONCE_SIZE, DesfireCommandState.Authenticate.RndB, &Buffer[1], (uint8_t*)&Key);
    /* Scrub the key */
    memset(&Key, 0, sizeof(Key));

    /* Done */
    DesfireState = DESFIRE_AUTHENTICATE2;
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    return DESFIRE_STATUS_RESPONSE_SIZE + DESFIRE_NONCE_SIZE;
}

static uint16_t MifareDesfireCmdAuthenticate2(uint8_t* Buffer, uint16_t ByteCount)
{
    MifareDesfireKeyType Key;

    /* Validate command length */
    if (ByteCount != 1 + 16) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Fetch the key */
    MemoryReadBlock(&Key, (uint16_t)&SelectedAppKeyAddress[DesfireCommandState.Authenticate.KeyId], sizeof(Key));
    /* Encipher to obtain plain text */
    CryptoEncrypt_3DES_KeyOption2_CBC(2 * DESFIRE_NONCE_SIZE, &Buffer[1], &Buffer[1], (uint8_t*)&Key);
    /* Now, RndA is at Buffer[1], RndB' is at Buffer[9] */
    if (memcmp(&Buffer[9], &DesfireCommandState.Authenticate.RndB[1], DESFIRE_NONCE_SIZE - 1) || (Buffer[16] != DesfireCommandState.Authenticate.RndB[0])) {
        /* Scrub the key */
        memset(&Key, 0, sizeof(Key));
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Compose the session key */
    /* NOTE: gcc does a very bad job optimising this. */
    SessionKey.Key1[0] = Buffer[1];
    SessionKey.Key1[1] = Buffer[2];
    SessionKey.Key1[2] = Buffer[3];
    SessionKey.Key1[3] = Buffer[4];
    SessionKey.Key1[4] = DesfireCommandState.Authenticate.RndB[0];
    SessionKey.Key1[5] = DesfireCommandState.Authenticate.RndB[1];
    SessionKey.Key1[6] = DesfireCommandState.Authenticate.RndB[2];
    SessionKey.Key1[7] = DesfireCommandState.Authenticate.RndB[3];
    SessionKey.Key2[0] = Buffer[5];
    SessionKey.Key2[1] = Buffer[6];
    SessionKey.Key2[2] = Buffer[7];
    SessionKey.Key2[3] = Buffer[8];
    SessionKey.Key2[4] = DesfireCommandState.Authenticate.RndB[4];
    SessionKey.Key2[5] = DesfireCommandState.Authenticate.RndB[5];
    SessionKey.Key2[6] = DesfireCommandState.Authenticate.RndB[6];
    SessionKey.Key2[7] = DesfireCommandState.Authenticate.RndB[7];
    AuthenticatedWithKey = DesfireCommandState.Authenticate.KeyId;
    /* Rotate the nonce A left by 8 bits */
    Buffer[9] = Buffer[1];
    /* Encipher the nonce A */
    CryptoEncrypt_3DES_KeyOption2_CBC(DESFIRE_NONCE_SIZE, &Buffer[2], &Buffer[1], (uint8_t*)&Key);
    /* Scrub the key */
    memset(&Key, 0, sizeof(Key));

    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + DESFIRE_NONCE_SIZE;
}

/*
 * DESFire application management commands
 */

static uint16_t MifareDesfireCmdGetApplicationIds2(uint8_t* Buffer, uint16_t ByteCount)
{
    MifareDesfireAppDirEntryType* Entry;
    uint8_t WriteIndex;

    WriteIndex = 1;
    for (Entry = DesfireCommandState.GetApplicationIds.NextEntry; Entry < &AppDir.Apps[MIFARE_DESFIRE_MAX_APPS]; ++Entry) {
        if (!Entry->Pointer)
            continue;
        if (WriteIndex >= 1 + MIFARE_DESFIRE_AID_SIZE * 19) {
            DesfireState = DESFIRE_GET_APPLICATION_IDS2;
            DesfireCommandState.GetApplicationIds.NextEntry = Entry;
            Buffer[0] = STATUS_ADDITIONAL_FRAME;
            return WriteIndex;
        }
        Buffer[WriteIndex++] = Entry->Aid[0];
        Buffer[WriteIndex++] = Entry->Aid[1];
        Buffer[WriteIndex++] = Entry->Aid[2];
    }

    /* Done */
    DesfireState = DESFIRE_IDLE;
    Buffer[0] = STATUS_OPERATION_OK;
    return WriteIndex;
}

static uint16_t MifareDesfireCmdGetApplicationIds1(uint8_t* Buffer, uint16_t ByteCount)
{
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Require zero AID to be selected */
    if (!MifareDesfireIsPiccAppSelected()) {
        /* TODO: Verify this is the proper error code */
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify authentication settings */
    if (!(Picc.MasterKeySettings & MIFARE_DESFIRE_FREE_DIRECTORY_LIST) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Setup the job and jump to the worker routine */
    DesfireCommandState.GetApplicationIds.NextEntry = &AppDir.Apps[0];
    return MifareDesfireCmdGetApplicationIds2(Buffer, ByteCount);
}

static uint16_t MifareDesfireCmdCreateApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    const MifareDesfireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };
    uint8_t Slot;
    uint8_t KeyCount;
    uint8_t AppPointer;
    uint8_t BlockCount;
    MifareDesfireAppDirEntryType* Entry;
    MifareDesfireApplicationType App;

    /* Validate command length */
    if (ByteCount != 1 + 3 + 1 + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    KeyCount = Buffer[4];
    /* Validate AID: AID of all zeros is reserved */
    if ((Aid[0] | Aid[1] | Aid[2]) == 0x00)
        goto invalid_args;
    /* Validate number of keys: less than max */
    if (KeyCount > MIFARE_DESFIRE_MAX_KEYS)
        goto invalid_args;
    /* Require zero AID to be selected */
    if (!MifareDesfireIsPiccAppSelected()) {
        /* TODO: Verify this is the proper error code */
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify authentication settings */
    if (!(Picc.MasterKeySettings & MIFARE_DESFIRE_FREE_CREATE_DELETE) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify this AID has not been allocated yet */
    if (MifareDesfireLookupAppByAid(Aid[0], Aid[1], Aid[2])) {
        Buffer[0] = STATUS_DUPLICATE_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Search for a free app slot */
    for (Slot = 0, Entry = &AppDir.Apps[0]; Slot < MIFARE_DESFIRE_MAX_APPS; ++Slot, ++Entry) {
        if (Entry->Pointer == 0)
            break;
    }

    if (Slot == MIFARE_DESFIRE_MAX_APPS) {
        Buffer[0] = STATUS_APP_COUNT_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Allocate storage for the application entry */
    BlockCount = 1 + MIFARE_DESFIRE_FILE_STATE_BLOCKS
        + (KeyCount * sizeof(MifareDesfireKeyType) + MIFARE_DESFIRE_EEPROM_BLOCK_SIZE - 1) / MIFARE_DESFIRE_EEPROM_BLOCK_SIZE;
    Entry->Pointer = AppPointer = MifareDesfireEepromAllocate(BlockCount);
    if (!AppPointer) {
        Buffer[0] = STATUS_OUT_OF_EEPROM_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    MemorySetBlock(0, AppPointer * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, BlockCount * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE);

    /* Initialise the application */
    App.MasterKeySettings = Buffer[5];
    App.KeyCount = KeyCount;
    MemoryWriteBlock(&App, AppPointer * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, sizeof(App));

    /* Update the directory */
    Entry->Aid[0] = Aid[0];
    Entry->Aid[1] = Aid[1];
    Entry->Aid[2] = Aid[2];
    MifareDesfireSynchronizeAppDir();

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;

invalid_args:
    Buffer[0] = STATUS_PARAMETER_ERROR;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t MifareDesfireCmdDeleteApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    MifareDesfireAppDirEntryType* Entry;
    MifareDesfireApplicationType App;

    /* Validate command length */
    if (ByteCount != 1 + 3) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate AID: AID of all zeros is reserved */
    if ((Buffer[1] | Buffer[2] | Buffer[3]) == 0x00) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Search for the app slot */
    Entry = MifareDesfireLookupAppByAid(Buffer[1], Buffer[2], Buffer[3]);
    if (!Entry) {
        Buffer[0] = STATUS_APP_NOT_FOUND;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    MemoryReadBlock(&App, Entry->Pointer * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, sizeof(App));

    /* Validate authentication */
    if (App.MasterKeySettings & MIFARE_DESFIRE_FREE_CREATE_DELETE) {
        /* Check whether PICC master key authentication is required */
        if (!(Picc.MasterKeySettings & MIFARE_DESFIRE_FREE_CREATE_DELETE) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
            /* PICC master key authentication is required */
            Buffer[0] = STATUS_AUTHENTICATION_ERROR;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
    }
    else {
        /* App settings take precedence, it seems */
        if (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
            Buffer[0] = STATUS_AUTHENTICATION_ERROR;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
    }

    if (Entry == SelectedAppEntry) {
        MifareDesfireSelectPiccApp();
    }

    /* Deactivate the app */
    Entry->Aid[0] = 0;
    Entry->Aid[1] = 0;
    Entry->Aid[2] = 0;
    Entry->Pointer = 0;
    MifareDesfireSynchronizeAppDir();

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t MifareDesfireCmdSelectApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    MifareDesfireAppDirEntryType* Entry;
    MifareDesfireApplicationType App;

    /* Validate command length */
    if (ByteCount != 1 + 3) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* AID of all zeros can be selected right away */
    if ((Buffer[1] | Buffer[2] | Buffer[3]) == 0x00) {
        MifareDesfireSelectPiccApp();
        Buffer[0] = STATUS_OPERATION_OK;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Search for the app slot */
    Entry = MifareDesfireLookupAppByAid(Buffer[1], Buffer[2], Buffer[3]);
    if (!Entry) {
        Buffer[0] = STATUS_APP_NOT_FOUND;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    SelectedAppEntry = Entry;
    MemoryReadBlock(&App, SelectedAppEntry->Pointer * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, sizeof(App));
    SelectedAppKeySettings = App.MasterKeySettings;
    SelectedAppKeyCount = App.KeyCount;
    SelectedAppKeyAddress = (MifareDesfireKeyType*)((SelectedAppEntry->Pointer + 1 + MIFARE_DESFIRE_FILE_STATE_BLOCKS) * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE);

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t MifareDesfireCmdGetKeySettings(uint8_t* Buffer, uint16_t ByteCount)
{
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    Buffer[1] = SelectedAppKeySettings;
    Buffer[2] = SelectedAppKeyCount;

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2;
}

/* Dispatching routines */

static uint16_t MifareDesfireProcessIdle(uint8_t* Buffer, uint16_t ByteCount)
{
    switch (Buffer[0]) {
    case CMD_GET_VERSION:
        return MifareDesfireCmdGetVersion1(Buffer, ByteCount);
    case CMD_FORMAT_PICC:
        return MifareDesfireCmdFormatPicc(Buffer, ByteCount);
    case CMD_AUTHENTICATE:
        return MifareDesfireCmdAuthenticate1(Buffer, ByteCount);
    case CMD_GET_APPLICATION_IDS:
        return MifareDesfireCmdGetApplicationIds1(Buffer, ByteCount);
    case CMD_CREATE_APPLICATION:
        return MifareDesfireCmdCreateApplication(Buffer, ByteCount);
    case CMD_DELETE_APPLICATION:
        return MifareDesfireCmdDeleteApplication(Buffer, ByteCount);
    case CMD_SELECT_APPLICATION:
        return MifareDesfireCmdSelectApplication(Buffer, ByteCount);
    case CMD_GET_KEY_SETTINGS:
        return MifareDesfireCmdGetKeySettings(Buffer, ByteCount);
    default:
        return ISO14443A_APP_NO_RESPONSE;
    }
}

static uint16_t MifareDesfireProcess(uint8_t* Buffer, uint16_t ByteCount)
{
    if (DesfireState == DESFIRE_IDLE)
        return MifareDesfireProcessIdle(Buffer, ByteCount);

    /* Expecting further data here */
    if (Buffer[0] != STATUS_ADDITIONAL_FRAME) {
        /* TODO: Should the card abort the current command? For now, yes. */
        DesfireState = DESFIRE_IDLE;
        Buffer[0] = STATUS_COMMAND_ABORTED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    switch (DesfireState) {
    case DESFIRE_GET_VERSION2:
        return MifareDesfireCmdGetVersion2(Buffer, ByteCount);
    case DESFIRE_GET_VERSION3:
        return MifareDesfireCmdGetVersion3(Buffer, ByteCount);
    case DESFIRE_GET_APPLICATION_IDS2:
        return MifareDesfireCmdGetApplicationIds2(Buffer, ByteCount);
    case DESFIRE_AUTHENTICATE2:
        return MifareDesfireCmdAuthenticate2(Buffer, ByteCount);
    default:
        return ISO14443A_APP_NO_RESPONSE;
    }
}

/*
 * ISO/IEC 14443-4 implementation
 * Currently this only supports wrapping things in 14443-4 I-blocks.
 */

void ISO144433AHalt(void);

typedef enum {
    ISO14443_4_STATE_EXPECT_RATS,
    ISO14443_4_STATE_ACTIVE,
} Iso144434StateType;

static Iso144434StateType Iso144434State;
static uint8_t Iso144434BlockNumber;

void ISO144434Init(void)
{
    Iso144434State = ISO14443_4_STATE_EXPECT_RATS;
    Iso144434BlockNumber = 1;
}

uint16_t ISO144434ProcessBlock(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t PCB = Buffer[0];
    uint8_t MyBlockNumber = Iso144434BlockNumber;

    /* Verify the block's length: at the very least PCB + CRCA */
    if (ByteCount < (1 + ISO14443A_CRCA_SIZE)) {
        /* Huh? Broken frame? */
        /* TODO: LOG ME */
        return ISO14443A_APP_NO_RESPONSE;
    }
    ByteCount -= 2;

    /* Verify the checksum; fail if doesn't match */
    if (!ISO14443ACheckCRCA(Buffer, ByteCount)) {
        LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, ByteCount);
        /* ISO/IEC 14443-4, clause 7.5.5. The PICC does not attempt any error recovery. */
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
        Buffer[0] = ATS_TL_BYTE;
        Buffer[1] = ATS_T0_BYTE;
        Buffer[2] = ATS_TA_BYTE;
        Buffer[3] = ATS_TB_BYTE;
        Buffer[4] = ATS_TC_BYTE;
        Buffer[5] = 0x00; /* T1: dummy value for historical bytes */
        Iso144434State = ISO14443_4_STATE_ACTIVE;
        ByteCount = ATS_FRAME_SIZE;
        break;

    case ISO14443_4_STATE_ACTIVE:
        /* See: ISO/IEC 14443-4; 7.1 Block format */
        switch (PCB & ISO14443_PCB_BLOCK_TYPE_MASK) {
        case ISO14443_PCB_I_BLOCK:
            if (PCB & (ISO14443_PCB_HAS_NAD_MASK | ISO14443_PCB_HAS_CID_MASK)) {
                /* Currently not supported => the frame is ignored */
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

            Buffer[0] = ISO14443_PCB_I_BLOCK_STATIC | MyBlockNumber;
            ByteCount = MifareDesfireProcess(Buffer + 1, ByteCount - 1);
            if (ByteCount == ISO14443A_APP_NO_RESPONSE) {
                return ByteCount;
            }
            break;

        case ISO14443_PCB_R_BLOCK:
            /* R-block handling is not supported */
            /* TODO: support at least retransmissions */
            return ISO14443A_APP_NO_RESPONSE;

        case ISO14443_PCB_S_BLOCK:
            if (PCB == ISO14443_PCB_S_DESELECT) {
                ISO144433AHalt();
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
    /* Something went wrong or we've received a halt command */
    ISO14443_3A_STATE_HALT,
    /* The card is powered up but not selected */
    ISO14443_3A_STATE_IDLE,
    /* Entered on REQA or WUPA; anticollision is being performed */
    ISO14443_3A_STATE_READY1,
    ISO14443_3A_STATE_READY2,
    /* Entered when the card has been selected */
    ISO14443_3A_STATE_ACTIVE,
} Iso144433AStateType;

static Iso144433AStateType Iso144433AState;
static Iso144433AStateType Iso144433AIdleState;

void ISO144433AInit(void)
{
    Iso144433AState = ISO14443_3A_STATE_IDLE;
}

void ISO144433AHalt(void)
{
    Iso144433AState = ISO14443_3A_STATE_HALT;
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
    case ISO14443_3A_STATE_IDLE:
        if (Cmd != ISO14443A_CMD_REQA)
            break;
        /* Fall-through */

    case ISO14443_3A_STATE_HALT:
        if (Cmd != ISO14443A_CMD_WUPA)
            break;
        Iso144433AIdleState = Iso144433AState;
        Iso144433AState = ISO14443_3A_STATE_READY1;
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
                Iso144433AState = ISO14443_3A_STATE_READY2;
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
                Iso144433AState = ISO14443_3A_STATE_ACTIVE;
            }

            return BitCount;
        }
        break;

    case ISO14443_3A_STATE_ACTIVE:
        /* Recognise the HLTA command */
        if (ISO144433AIsHalt(Buffer, BitCount)) {
            LogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
            Iso144433AState = ISO14443_3A_STATE_HALT;
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Forward to ISO/IEC 14443-4 processing code */
        return ISO144434ProcessBlock(Buffer, BitCount / 8) * 8;
    }

    /* Unknown command. Reset back to idle/halt state. */
    Iso144433AState = Iso144433AIdleState;
    return ISO14443A_APP_NO_RESPONSE;
}

void MifareDesfireAppInit(void)
{
    /* Init lower layers */
    ISO144433AInit();
    ISO144434Init();
    /* Init DESFire junk */
    MemoryReadBlock(&Picc, MIFARE_DESFIRE_PICC_INFO_BLOCK_ID * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, sizeof(Picc));
    if (Picc.FirstFreeBlock == 0) {
        MifareDesfireFormatPicc();
    }
    else {
        MemoryReadBlock(&AppDir, MIFARE_DESFIRE_APP_DIR_BLOCK_ID * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, sizeof(AppDir));
    }
    MifareDesfireSelectPiccApp();
    DesfireState = DESFIRE_IDLE;
}

void MifareDesfireAppReset(void)
{
    MifareDesfireAppInit();
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
    memcpy(Uid, Picc.Uid, ActiveConfiguration.UidSize);
}

void MifareDesfireSetUid(ConfigurationUidType Uid)
{
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
