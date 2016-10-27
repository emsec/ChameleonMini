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

#define DEBUG_THIS

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

#else

#define DEBUG_PRINT(...)

#endif

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

#define GET_LE16(p) \
    ((uint16_t)((p)[0] | ((p)[1] << 8)))

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

    STATUS_PICC_INTEGRITY_ERROR = 0xC1,
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

#define DESFIRE_PICC_APP_INDEX 0

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
    DESFIRE_READING_DATA,
    DESFIRE_WRITING_DATA,
} DesfireStateType;

typedef union {
    struct {
        uint8_t KeyId;
        uint8_t RndB[CRYPTO_DES_KEY_SIZE];
    } Authenticate;
    struct {
        uint8_t NextIndex;
    } GetApplicationIds;
    struct {
        uint8_t BlockId;
        uint8_t Offset;
        uint16_t RemainingBytes;
    } XxxDataFile;
} DesfireSavedCommandStateType;

static DesfireStateType DesfireState;
static DesfireSavedCommandStateType DesfireCommandState;
static uint16_t CardCapacityBlocks;
static uint8_t AuthenticatedWithKey;
static MifareDesfireKeyType SessionKey;

/* Cached data: flush to FRAM if changed */
static MifareDesfirePiccInfoType Picc;
static MifareDesfireAppDirType AppDir;

static uint8_t SelectedAppSlot;
static uint8_t SelectedAppKeySettings;
static uint8_t SelectedAppKeyCount;
static MifareDesfireFileType* SelectedAppFilesAddress; /* in FRAM */
static MifareDesfireKeyType* SelectedAppKeyAddress; /* in FRAM */

static void ReadBlocks(void* Buffer, uint8_t StartBlock, uint16_t Count);
static void WriteBlocks(void* Buffer, uint8_t StartBlock, uint16_t Count);


static void SynchronizeAppDir(void)
{
    WriteBlocks(&AppDir, MIFARE_DESFIRE_APP_DIR_BLOCK_ID, sizeof(MifareDesfireAppDirType));
}

static void SynchronizePiccInfo(void)
{
    WriteBlocks(&Picc, MIFARE_DESFIRE_PICC_INFO_BLOCK_ID, sizeof(Picc));
}


static uint8_t GetCardCapacityBlocks(void)
{
    uint8_t MaxFreeBlocks;

    switch (Picc.StorageSize) {
    case MIFARE_DESFIRE_STORAGE_SIZE_2K:
        MaxFreeBlocks = 0x40 - MIFARE_DESFIRE_FIRST_FREE_BLOCK_ID;
        break;
    case MIFARE_DESFIRE_STORAGE_SIZE_4K:
        MaxFreeBlocks = 0x80 - MIFARE_DESFIRE_FIRST_FREE_BLOCK_ID;
        break;
    case MIFARE_DESFIRE_STORAGE_SIZE_8K:
        MaxFreeBlocks = 0 - MIFARE_DESFIRE_FIRST_FREE_BLOCK_ID;
        break;
    default:
        return 0;
    }

    return MaxFreeBlocks - Picc.FirstFreeBlock;
}


static uint8_t AllocateBlocks(uint8_t BlockCount)
{
    uint8_t Block;

    /* Check if we have space */
    Block = Picc.FirstFreeBlock;
    if (Block + BlockCount < Block) {
        return 0;
    }
    Picc.FirstFreeBlock = Block + BlockCount;
    SynchronizePiccInfo();
    MemorySetBlock(0x00, Block * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, BlockCount * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE);
    return Block;
}

static void ReadBlocks(void* Buffer, uint8_t StartBlock, uint16_t Count)
{
    MemoryReadBlock(Buffer, StartBlock * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, Count);
}

static void WriteBlocks(void* Buffer, uint8_t StartBlock, uint16_t Count)
{
    MemoryWriteBlock(Buffer, StartBlock * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE, Count);
}


static uint8_t GetAppProperty(uint8_t BlockId, uint8_t AppSlot)
{
    MifareDesfireApplicationDataType Data;

    ReadBlocks(&Data, BlockId, sizeof(Data));
    return Data.AppData[AppSlot];
}

static void SetAppProperty(uint8_t BlockId, uint8_t AppSlot, uint8_t Value)
{
    MifareDesfireApplicationDataType Data;

    ReadBlocks(&Data, BlockId, sizeof(Data));
    Data.AppData[AppSlot] = Value;
    WriteBlocks(&Data, BlockId, sizeof(Data));
}

static uint8_t GetAppKeySettings(uint8_t AppSlot)
{
    return GetAppProperty(MIFARE_DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot);
}

static void SetAppKeySettings(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(MIFARE_DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot, Value);
}

static void SetAppKeyCount(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(MIFARE_DESFIRE_APP_KEY_COUNT_BLOCK_ID, AppSlot, Value);
}

static void SetAppKeyStorageBlockId(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(MIFARE_DESFIRE_APP_KEYS_PTR_BLOCK_ID, AppSlot, Value);
}

static uint8_t GetAppFileIndexBlockId(uint8_t AppSlot)
{
    return GetAppProperty(MIFARE_DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot);
}

static void SetAppFileIndexBlockId(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(MIFARE_DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot, Value);
}


static void ReadSelectedAppKey(uint8_t KeyId, uint8_t* Key)
{
    MemoryReadBlock(Key, (uint16_t)&SelectedAppKeyAddress[KeyId], sizeof(MifareDesfireKeyType));
}

static void WriteSelectedAppKey(uint8_t KeyId, const uint8_t* Key)
{
    MemoryWriteBlock(Key, (uint16_t)&SelectedAppKeyAddress[KeyId], sizeof(MifareDesfireKeyType));
}

static uint8_t LookupAppSlot(const MifareDesfireAidType Aid)
{
    uint8_t Slot;

    for (Slot = 0; Slot < MIFARE_DESFIRE_MAX_SLOTS; ++Slot) {
        if (!memcmp(AppDir.AppIds[Slot], Aid, MIFARE_DESFIRE_AID_SIZE))
            break;
    }

    return Slot;
}

static void SelectAppBySlot(uint8_t AppSlot)
{
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    SelectedAppSlot = AppSlot;

    SelectedAppKeySettings = GetAppProperty(MIFARE_DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot);
    SelectedAppKeyCount = GetAppProperty(MIFARE_DESFIRE_APP_KEY_COUNT_BLOCK_ID, AppSlot);
    SelectedAppFilesAddress = (MifareDesfireFileType*)(GetAppProperty(MIFARE_DESFIRE_APP_KEYS_PTR_BLOCK_ID, AppSlot) * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE);
    SelectedAppKeyAddress = (MifareDesfireKeyType*)(GetAppProperty(MIFARE_DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot) * MIFARE_DESFIRE_EEPROM_BLOCK_SIZE);
}

static uint8_t SelectAppByAid(const MifareDesfireAidType Aid)
{
    uint8_t Slot;

    /* Search for the app slot */
    Slot = LookupAppSlot(Aid);
    if (Slot == MIFARE_DESFIRE_MAX_SLOTS) {
        return STATUS_APP_NOT_FOUND;
    }

    SelectAppBySlot(Slot + 1);
    return STATUS_OPERATION_OK;
}

static uint8_t CreateApp(const MifareDesfireAidType Aid, uint8_t KeyCount, uint8_t KeySettings)
{
    uint8_t Slot;
    uint8_t FreeSlot;
    uint8_t KeysBlockId, FilesBlockId;

    /* Verify this AID has not been allocated yet */
    if (LookupAppSlot(Aid) != MIFARE_DESFIRE_MAX_SLOTS) {
        return STATUS_DUPLICATE_ERROR;
    }
    /* Verify there is space */
    Slot = AppDir.FirstFreeSlot;
    if (Slot == MIFARE_DESFIRE_MAX_SLOTS) {
        return STATUS_APP_COUNT_ERROR;
    }
    /* Update the next free slot */
    for (FreeSlot = Slot + 1; FreeSlot < MIFARE_DESFIRE_MAX_SLOTS; ++FreeSlot) {
        if ((AppDir.AppIds[FreeSlot][0] | AppDir.AppIds[FreeSlot][1] | AppDir.AppIds[FreeSlot][2]) == 0)
            break;
    }

    /* Allocate storage for the application */
    KeysBlockId = AllocateBlocks(MIFARE_DESFIRE_BYTES_TO_BLOCKS(KeyCount * sizeof(MifareDesfireKeyType)));
    if (!KeysBlockId) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    }
    FilesBlockId = AllocateBlocks(MIFARE_DESFIRE_FILE_INDEX_BLOCKS);
    if (!FilesBlockId) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    }

    /* Initialise the application */
    SetAppKeySettings(Slot + 1, KeySettings);
    SetAppKeyCount(Slot + 1, KeyCount);
    SetAppKeyStorageBlockId(Slot + 1, KeysBlockId);
    SetAppFileIndexBlockId(Slot + 1, FilesBlockId);

    /* Update the directory */
    AppDir.FirstFreeSlot = FreeSlot;
    AppDir.AppIds[Slot][0] = Aid[0];
    AppDir.AppIds[Slot][1] = Aid[1];
    AppDir.AppIds[Slot][2] = Aid[2];
    SynchronizeAppDir();

    return STATUS_OPERATION_OK;
}

static uint8_t DeleteApp(const MifareDesfireAidType Aid)
{
    uint8_t Slot;

    /* Search for the app slot */
    Slot = LookupAppSlot(Aid);
    if (Slot == MIFARE_DESFIRE_MAX_SLOTS) {
        return STATUS_APP_NOT_FOUND;
    }
    /* Deactivate the app */
    AppDir.AppIds[Slot][0] = 0;
    AppDir.AppIds[Slot][1] = 0;
    AppDir.AppIds[Slot][2] = 0;
    if (Slot < AppDir.FirstFreeSlot) {
        AppDir.FirstFreeSlot = Slot;
    }
    SynchronizeAppDir();

    if (Slot == SelectedAppSlot) {
        SelectAppBySlot(DESFIRE_PICC_APP_INDEX);
    }
    return STATUS_OPERATION_OK;
}


static void CreatePiccApp(void)
{
    SetAppKeySettings(DESFIRE_PICC_APP_INDEX, 0x0F);
    SetAppKeyCount(DESFIRE_PICC_APP_INDEX, 1);
    SetAppKeyStorageBlockId(DESFIRE_PICC_APP_INDEX, AllocateBlocks(1));
}

static void SelectPiccApp(void)
{
    SelectAppBySlot(DESFIRE_PICC_APP_INDEX);
}

static uint8_t IsPiccAppSelected(void)
{
    return SelectedAppSlot == DESFIRE_PICC_APP_INDEX;
}


static void FormatPicc(void)
{
    /* Wipe application directory */
    memset(&AppDir, 0, sizeof(AppDir));
    /* Reset the free block pointer */
    Picc.FirstFreeBlock = MIFARE_DESFIRE_FIRST_FREE_BLOCK_ID;

    SynchronizePiccInfo();
    SynchronizeAppDir();
}

static void FactoryFormatPiccEV0(void)
{
    /* Wipe PICC data */
    memset(&Picc, 0xFF, sizeof(Picc));
    memset(&Picc.Uid[0], 0x00, MIFARE_DESFIRE_UID_SIZE);
    /* Initialize params to look like EV0 */
    Picc.StorageSize = MIFARE_DESFIRE_STORAGE_SIZE_4K;
    Picc.HwVersionMajor = MIFARE_DESFIRE_HW_MAJOR_EV0;
    Picc.HwVersionMinor = MIFARE_DESFIRE_HW_MINOR_EV0;
    Picc.SwVersionMajor = MIFARE_DESFIRE_SW_MAJOR_EV0;
    Picc.SwVersionMinor = MIFARE_DESFIRE_SW_MINOR_EV0;
    /* Initialize the root app data */
    CreatePiccApp();
    /* Continue with user data initialization */
    FormatPicc();
}


static uint8_t CreateStandardFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize)
{
    uint8_t FileIndexBlock;
    uint8_t FileIndex[MIFARE_DESFIRE_MAX_FILES];
    uint16_t BlockCount;
    uint8_t BlockId;
    MifareDesfireFileType File;

    /* Read in the file index */
    FileIndexBlock = GetAppFileIndexBlockId(SelectedAppSlot);
    ReadBlocks(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    /* Check if the file already exists */
    if (FileIndex[FileNum]) {
        return STATUS_DUPLICATE_ERROR;
    }
    /* Allocate blocks for the file */
    BlockCount = MIFARE_DESFIRE_BYTES_TO_BLOCKS(FileSize);
    BlockId = AllocateBlocks(1 + BlockCount);
    if (!BlockId) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    }
    /* Fill in the control structure and write it */
    File.Type = MIFARE_DESFIRE_FILE_STANDARD_DATA;
    File.CommSettings = CommSettings;
    File.AccessRights = AccessRights;
    File.StandardFile.FileSize = FileSize;
    WriteBlocks(&File, BlockId, sizeof(File));
    /* Write the file index */
    FileIndex[FileNum] = BlockId;
    WriteBlocks(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    return STATUS_OPERATION_OK;
}

static uint8_t CreateBackupFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize)
{
    uint8_t FileIndexBlock;
    uint8_t FileIndex[MIFARE_DESFIRE_MAX_FILES];
    uint16_t BlockCount;
    uint8_t BlockId;
    MifareDesfireFileType File;

    /* Read in the file index */
    FileIndexBlock = GetAppFileIndexBlockId(SelectedAppSlot);
    ReadBlocks(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    /* Check if the file already exists */
    if (FileIndex[FileNum]) {
        return STATUS_DUPLICATE_ERROR;
    }
    /* Allocate blocks for the file */
    BlockCount = MIFARE_DESFIRE_BYTES_TO_BLOCKS(FileSize);
    BlockId = AllocateBlocks(1 + 2 * BlockCount);
    if (!BlockId) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    }
    /* Fill in the control structure and write it */
    File.Type = MIFARE_DESFIRE_FILE_BACKUP_DATA;
    File.CommSettings = CommSettings;
    File.AccessRights = AccessRights;
    File.BackupFile.FileSize = FileSize;
    File.BackupFile.BlockCount = BlockCount;
    File.BackupFile.Dirty = false;
    WriteBlocks(&File, BlockId, sizeof(File));
    /* Write the file index */
    FileIndex[FileNum] = BlockId;
    WriteBlocks(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    return STATUS_OPERATION_OK;
}

static uint8_t DeleteFile(uint8_t FileNum)
{
    uint8_t FileIndexBlock;
    uint8_t FileIndex[MIFARE_DESFIRE_MAX_FILES];

    FileIndexBlock = GetAppFileIndexBlockId(SelectedAppSlot);
    ReadBlocks(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    if (FileIndex[FileNum]) {
        FileIndex[FileNum] = 0;
    }
    else {
        return STATUS_FILE_NOT_FOUND;
    }
    WriteBlocks(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    return STATUS_OPERATION_OK;
}


/*
 * DESFire general commands
 */

static uint16_t EV0CmdGetVersion1(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = Picc.Uid[0];
    Buffer[2] = DESFIRE_TYPE;
    Buffer[3] = DESFIRE_SUBTYPE;
    Buffer[4] = Picc.HwVersionMajor;
    Buffer[4] = Picc.HwVersionMinor;
    Buffer[6] = Picc.StorageSize;
    Buffer[7] = DESFIRE_HW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION2;
    return 8;
}

static uint16_t EV0CmdGetVersion2(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = Picc.Uid[0];
    Buffer[2] = DESFIRE_TYPE;
    Buffer[3] = DESFIRE_SUBTYPE;
    Buffer[4] = Picc.SwVersionMajor;
    Buffer[4] = Picc.SwVersionMinor;
    Buffer[6] = Picc.StorageSize;
    Buffer[7] = DESFIRE_SW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION3;
    return 8;
}

static uint16_t EV0CmdGetVersion3(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_OPERATION_OK;
    /* UID/Serial number; does not depend on card mode */
    memcpy(&Buffer[1], &Picc.Uid[0], MIFARE_DESFIRE_UID_SIZE);
    Buffer[8] = Picc.BatchNumber[0];
    Buffer[9] = Picc.BatchNumber[1];
    Buffer[10] = Picc.BatchNumber[2];
    Buffer[11] = Picc.BatchNumber[3];
    Buffer[12] = Picc.BatchNumber[4];
    Buffer[13] = Picc.ProductionWeek;
    Buffer[14] = Picc.ProductionYear;
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
    ReadSelectedAppKey(KeyId, Key);
    /* Generate the nonce B */
    RandomGetBuffer(DesfireCommandState.Authenticate.RndB, DESFIRE_NONCE_SIZE);
    /* Encipher the nonce B with the selected key */
    CryptoEncrypt_2KTDEA_CBC(DESFIRE_NONCE_SIZE / CRYPTO_DES_BLOCK_SIZE, DesfireCommandState.Authenticate.RndB, &Buffer[1], Key);
    /* Scrub the key */
    memset(&Key, 0, sizeof(Key));

    /* Done */
    DesfireState = DESFIRE_AUTHENTICATE2;
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    return DESFIRE_STATUS_RESPONSE_SIZE + DESFIRE_NONCE_SIZE;
}

static uint16_t EV0CmdAuthenticate2KTDEA2(uint8_t* Buffer, uint16_t ByteCount)
{
    MifareDesfireKeyType Key;

    /* Validate command length */
    if (ByteCount != 1 + 16) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Fetch the key */
    ReadSelectedAppKey(DesfireCommandState.Authenticate.KeyId, Key);
    /* Encipher to obtain plain text */
    CryptoEncrypt_2KTDEA_CBC(2 * DESFIRE_NONCE_SIZE / CRYPTO_DES_BLOCK_SIZE, &Buffer[1], &Buffer[1], Key);
    /* Now, RndA is at Buffer[1], RndB' is at Buffer[9] */
    if (memcmp(&Buffer[9], &DesfireCommandState.Authenticate.RndB[1], DESFIRE_NONCE_SIZE - 1) || (Buffer[16] != DesfireCommandState.Authenticate.RndB[0])) {
        /* Scrub the key */
        memset(&Key, 0, sizeof(Key));
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Compose the session key */
    /* NOTE: gcc does a very bad job optimising this. */
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
    /* Encipher the nonce A */
    CryptoEncrypt_2KTDEA_CBC(DESFIRE_NONCE_SIZE / CRYPTO_DES_BLOCK_SIZE, &Buffer[2], &Buffer[1], Key);
    /* Scrub the key */
    memset(&Key, 0, sizeof(Key));

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + DESFIRE_NONCE_SIZE;
}

static uint16_t EV0CmdChangeKey(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t KeyId;
    uint8_t ChangeKeyId;

    /* Validate command length */
    if (ByteCount != 1 + 1 + 24) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    KeyId = Buffer[1];
    /* Validate number of keys: less than max */
    if (KeyId >= SelectedAppKeyCount) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate the state against change key settings */
    ChangeKeyId = SelectedAppKeySettings >> 4;
    switch (ChangeKeyId) {
    case MIFARE_DESFIRE_ALL_KEYS_FROZEN:
        /* Only master key may be (potentially) changed */
        if (KeyId != DESFIRE_MASTER_KEY_ID || !(SelectedAppKeySettings & MIFARE_DESFIRE_ALLOW_MASTER_KEY_CHANGE)) {
            Buffer[0] = STATUS_PERMISSION_DENIED;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
        break;
    case MIFARE_DESFIRE_USE_TARGET_KEY:
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
    CryptoEncrypt_2KTDEA_CBC(3, &Buffer[2], &Buffer[2], SessionKey);
    /* Verify the checksum first */
    if (!ISO14443ACheckCRCA(&Buffer[2], sizeof(MifareDesfireKeyType))) {
        Buffer[0] = STATUS_INTEGRITY_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* The PCD generates data differently based on whether AuthKeyId == ChangeKeyId */
    if (KeyId != AuthenticatedWithKey) {
        MifareDesfireKeyType OldKey;
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
        if (!ISO14443ACheckCRCA(&Buffer[2], sizeof(MifareDesfireKeyType))) {
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
    memset(&Buffer[2], 0, sizeof(MifareDesfireKeyType));

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

    Buffer[1] = SelectedAppKeySettings;
    Buffer[2] = SelectedAppKeyCount;

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2;
}

static uint16_t EV0CmdChangeKeySettings(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t NewSettings;

    /* Validate command length */
    if (ByteCount != 1 + 8) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify whether settings are changeable */
    if (!(SelectedAppKeySettings & MIFARE_DESFIRE_ALLOW_CONFIG_CHANGE)) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify the master key has been authenticated with */
    if (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Encipher to obtain plaintext */
    CryptoEncrypt_2KTDEA_CBC(3, &Buffer[2], &Buffer[2], SessionKey);
    /* Verify the checksum first */
    if (!ISO14443ACheckCRCA(&Buffer[2], sizeof(MifareDesfireKeyType))) {
        Buffer[0] = STATUS_INTEGRITY_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    NewSettings = Buffer[1];
    if (IsPiccAppSelected()) {
        NewSettings &= 0x0F;
    }
    SetAppKeySettings(SelectedAppSlot, NewSettings);

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2;
}

/*
 * DESFire application management commands
 */

static uint16_t EV0CmdGetApplicationIds2(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t EntryIndex;
    uint8_t WriteIndex;

    WriteIndex = 1;
    for (EntryIndex = DesfireCommandState.GetApplicationIds.NextIndex; EntryIndex < MIFARE_DESFIRE_MAX_SLOTS; ++EntryIndex) {
        if ((AppDir.AppIds[EntryIndex][0] | AppDir.AppIds[EntryIndex][1] | AppDir.AppIds[EntryIndex][2]) == 0)
            continue;
        if (WriteIndex >= 1 + MIFARE_DESFIRE_AID_SIZE * 19) {
            DesfireState = DESFIRE_GET_APPLICATION_IDS2;
            DesfireCommandState.GetApplicationIds.NextIndex = EntryIndex;
            Buffer[0] = STATUS_ADDITIONAL_FRAME;
            return WriteIndex;
        }
        Buffer[WriteIndex++] = AppDir.AppIds[EntryIndex][0];
        Buffer[WriteIndex++] = AppDir.AppIds[EntryIndex][1];
        Buffer[WriteIndex++] = AppDir.AppIds[EntryIndex][2];
    }

    /* Done */
    DesfireState = DESFIRE_IDLE;
    Buffer[0] = STATUS_OPERATION_OK;
    return WriteIndex;
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
    if (!(SelectedAppKeySettings & MIFARE_DESFIRE_FREE_DIRECTORY_LIST) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Setup the job and jump to the worker routine */
    DesfireCommandState.GetApplicationIds.NextIndex = 1;
    return EV0CmdGetApplicationIds2(Buffer, ByteCount);
}

static uint16_t EV0CmdCreateApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    const MifareDesfireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };

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
    /* Validate number of keys: less than max */
    if (Buffer[4] > MIFARE_DESFIRE_MAX_KEYS) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    /* Verify authentication settings */
    if (!(SelectedAppKeySettings & MIFARE_DESFIRE_FREE_CREATE_DELETE) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        /* PICC master key authentication is required */
        Status = STATUS_AUTHENTICATION_ERROR;
        goto exit_with_status;
    }
    /* Done */
    Status = CreateApp(Aid, Buffer[4], Buffer[5]);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdDeleteApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    const MifareDesfireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };
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
        PiccKeySettings = GetAppKeySettings(DESFIRE_PICC_APP_INDEX);
        /* Check the PICC key settings whether it is OK to delete using app master key */
        if (!(PiccKeySettings & MIFARE_DESFIRE_FREE_CREATE_DELETE)) {
            Status = STATUS_AUTHENTICATION_ERROR;
            goto exit_with_status;
        }
    }

    /* Done */
    Status = DeleteApp(Aid);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdSelectApplication(uint8_t* Buffer, uint16_t ByteCount)
{
    const MifareDesfireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };

    /* Validate command length */
    if (ByteCount != 1 + 3) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Done */
    Buffer[0] = SelectAppByAid(Aid);
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire application file management commands
 */

static uint8_t CreateFileCommonValidation(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights)
{
    /* Validate file number */
    if (FileNum >= MIFARE_DESFIRE_MAX_FILES) {
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
    uint16_t FileSize;

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
    FileSize = Buffer[5] | (Buffer[6] << 8);
    if (Buffer[7] || FileSize >= 8160) {
        Status = STATUS_OUT_OF_EEPROM_ERROR;
        goto exit_with_status;
    }

    Status = CreateStandardFile(FileNum, CommSettings, AccessRights, FileSize);

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
    uint16_t FileSize;

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
    FileSize = Buffer[5] | (Buffer[6] << 8);
    if (Buffer[7] || FileSize >= 4096) {
        Status = STATUS_OUT_OF_EEPROM_ERROR;
        goto exit_with_status;
    }

    Status = CreateBackupFile(FileNum, CommSettings, AccessRights, FileSize);

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
    if (FileNum >= MIFARE_DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    /* Validate access settings */
    if (!(SelectedAppKeySettings & MIFARE_DESFIRE_FREE_CREATE_DELETE) && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
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

static void EncipherData(uint8_t* Buffer, uint16_t ByteCount)
{
    /* IMPLEMENT ME */
}

static uint8_t DecipherData(uint8_t* Buffer, uint16_t ByteCount)
{
    /* IMPLEMENT ME */
    return STATUS_OPERATION_OK;
}

static uint8_t ReadDataFileIterator(void)
{
    /* IMPLEMENT ME */
    return STATUS_OPERATION_OK;
}

static uint8_t ReadDataFile(uint8_t FileNum, uint16_t Offset, uint16_t ByteCount)
{
    uint8_t FileIndexBlock;
    uint8_t FileIndex[MIFARE_DESFIRE_MAX_FILES];
    uint8_t BlockId;
    MifareDesfireFileType File;
    uint8_t RequiredKeyId1, RequiredKeyId2;
    uint8_t CommSettings;

    /* Read in the file index */
    FileIndexBlock = GetAppFileIndexBlockId(SelectedAppSlot);
    ReadBlocks(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    /* Check whether the file exists */
    BlockId = FileIndex[FileNum];
    if (!BlockId) {
        return STATUS_FILE_NOT_FOUND;
    }
    /* Read the file control block */
    ReadBlocks(&File, BlockId, sizeof(File));
    CommSettings = File.CommSettings;
    /* Verify authentication: read or read&write required */
    RequiredKeyId1 = (File.AccessRights >> MIFARE_DESFIRE_READWRITE_ACCESS_RIGHTS_SHIFT) & 0x0F;
    RequiredKeyId2 = (File.AccessRights >> MIFARE_DESFIRE_READ_ACCESS_RIGHTS_SHIFT) & 0x0F;
    if (AuthenticatedWithKey == DESFIRE_NOT_AUTHENTICATED) {
        /* Require free access */
        if (RequiredKeyId1 != MIFARE_DESFIRE_ACCESS_FREE && RequiredKeyId2 != MIFARE_DESFIRE_ACCESS_FREE) {
            return STATUS_AUTHENTICATION_ERROR;
        }
        /* Force plain comms, as we have no key anyways */
        CommSettings = MIFARE_DESFIRE_COMMS_PLAINTEXT;
    }
    else {
        if (AuthenticatedWithKey != RequiredKeyId1 && AuthenticatedWithKey != RequiredKeyId2) {
            return STATUS_AUTHENTICATION_ERROR;
        }
        /* We have a valid key, so we can CMAC/encipher as required by comms settings */
    }

    /* Prepare the transfer and start it */
    DesfireCommandState.XxxDataFile.BlockId = BlockId + 1 + Offset / MIFARE_DESFIRE_EEPROM_BLOCK_SIZE;
    DesfireCommandState.XxxDataFile.Offset = Offset & (MIFARE_DESFIRE_EEPROM_BLOCK_SIZE - 1);
    DesfireCommandState.XxxDataFile.RemainingBytes = ByteCount;
    return ReadDataFileIterator();
}

static uint16_t EV0CmdReadData(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t Status;
    uint8_t FileNum;
    uint16_t Offset;
    uint16_t Length;
    uint16_t CardCapacity;

    /* Validate command length */
    if (ByteCount != 1 + 1 + 3 + 3) {
        Status = STATUS_LENGTH_ERROR;
        goto exit_with_status;
    }
    FileNum = Buffer[1];
    /* Validate file number */
    if (FileNum >= MIFARE_DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        goto exit_with_status;
    }
    /* Validate offset and length (preliminary) */
    Offset = GET_LE16(&Buffer[2]);
    Length = GET_LE16(&Buffer[5]);
    if (Buffer[4] || Buffer[7]) {
        Status = STATUS_BOUNDARY_ERROR;
        goto exit_with_status;
    }

    Status = ReadDataFile(FileNum, Offset, Length);

exit_with_status:
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdWriteData(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

static uint16_t EV0CmdGetValue(uint8_t* Buffer, uint16_t ByteCount)
{
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
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
    default:
        Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
}

static uint16_t MifareDesfireProcessCommand(uint8_t* Buffer, uint16_t ByteCount)
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
        return EV0CmdGetVersion2(Buffer, ByteCount);
    case DESFIRE_GET_VERSION3:
        return EV0CmdGetVersion3(Buffer, ByteCount);
    case DESFIRE_GET_APPLICATION_IDS2:
        return EV0CmdGetApplicationIds2(Buffer, ByteCount);
    case DESFIRE_AUTHENTICATE2:
        return EV0CmdAuthenticate2KTDEA2(Buffer, ByteCount);
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
        memmove(&Buffer[2], &Buffer[5], ByteCount);
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
static uint8_t Iso144434CardID;

void ISO144434Init(void)
{
    Iso144434State = ISO14443_4_STATE_EXPECT_RATS;
    Iso144434BlockNumber = 1;
}

uint16_t ISO144434ProcessBlock(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t PCB = Buffer[0];
    uint8_t MyBlockNumber = Iso144434BlockNumber;
    uint8_t PrologueLength;

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
        DEBUG_PRINT("\r\nISO14443-4: In EXPECT_RATS, RX'd %02X", Buffer[0]);
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
        PrologueLength = 1;
        if (PCB & ISO14443_PCB_HAS_CID_MASK) {
            if (Buffer[1] != Iso144434CardID) {
                /* Different card ID => the frame is ignored */
                return ISO14443A_APP_NO_RESPONSE;
            }
            PrologueLength = 2;
        }
        switch (PCB & ISO14443_PCB_BLOCK_TYPE_MASK) {
        case ISO14443_PCB_I_BLOCK:
            if (PCB & ISO14443_PCB_HAS_NAD_MASK) {
                /* Not supported => the frame is ignored */
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
            if (PCB & ISO14443_PCB_HAS_CID_MASK) {
                Buffer[0] |= ISO14443_PCB_HAS_CID_MASK;
                Buffer[1] = Iso144434CardID;
            }
            ByteCount = MifareDesfireProcess(Buffer + PrologueLength, ByteCount - PrologueLength);
            if (ByteCount == ISO14443A_APP_NO_RESPONSE) {
                return ISO14443A_APP_NO_RESPONSE;
            }
            ByteCount += PrologueLength;
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
    case ISO14443_3A_STATE_HALT:
        DEBUG_PRINT("\r\nISO14443-3: In HALT, RX'd %02X", Cmd);
        if (Cmd != ISO14443A_CMD_WUPA) {
            break;
        }
        /* Fall-through */

    case ISO14443_3A_STATE_IDLE:
        DEBUG_PRINT("\r\nISO14443-3: In IDLE, RX'd %02X", Cmd);
        if (Cmd != ISO14443A_CMD_REQA && Cmd != ISO14443A_CMD_WUPA) {
            break;
        }

        Iso144433AIdleState = Iso144433AState;
        Iso144433AState = ISO14443_3A_STATE_READY1;
        Buffer[0] = (ATQA_VALUE >> 0) & 0x00FF;
        Buffer[1] = (ATQA_VALUE >> 8) & 0x00FF;
        return ISO14443A_ATQA_FRAME_SIZE;

    case ISO14443_3A_STATE_READY1:
        DEBUG_PRINT("\r\nISO14443-3: In READY1, RX'd %02X", Cmd);
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
        DEBUG_PRINT("\r\nISO14443-3: In READY2, RX'd %02X", Cmd);
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
        DEBUG_PRINT("\r\nISO14443-3: In ACTIVE, RX'd %02X", Cmd);
        /* Recognise the HLTA command */
        if (ISO144433AIsHalt(Buffer, BitCount)) {
            LogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
            Iso144433AState = ISO14443_3A_STATE_HALT;
            DEBUG_PRINT("\r\nISO14443-3: Got HALT");
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Forward to ISO/IEC 14443-4 processing code */
        return ISO144434ProcessBlock(Buffer, (BitCount + 7) / 8) * 8;
    }

    /* Unknown command. Reset back to idle/halt state. */
    DEBUG_PRINT("\r\nISO14443-3: RESET TO IDLE");
    Iso144433AState = Iso144433AIdleState;
    return ISO14443A_APP_NO_RESPONSE;
}

void RunTDEATests();

void MifareDesfireAppInit(void)
{
    RunTDEATests();

    /* Init lower layers */
    ISO144433AInit();
    ISO144434Init();
    /* Init DESFire junk */
    ReadBlocks(&Picc, MIFARE_DESFIRE_PICC_INFO_BLOCK_ID, sizeof(Picc));
    if (Picc.Uid[0] == 0xFF && Picc.Uid[1] == 0xFF && Picc.Uid[2] == 0xFF && Picc.Uid[3] == 0xFF) {
        DEBUG_PRINT("\r\nFactory resetting the device\r\n");
        FactoryFormatPiccEV0();
    }
    else {
        ReadBlocks(&AppDir, MIFARE_DESFIRE_APP_DIR_BLOCK_ID, sizeof(AppDir));
    }
    CardCapacityBlocks = GetCardCapacityBlocks();
    SelectPiccApp();
    DesfireState = DESFIRE_IDLE;
}

void MifareDesfireAppReset(void)
{
    /* This is called repeatedly, so limit the amount of work done */
    SelectPiccApp();
    DesfireState = DESFIRE_IDLE;
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
    memcpy(Picc.Uid, Uid, ActiveConfiguration.UidSize);
    SynchronizePiccInfo();
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
