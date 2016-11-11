/*
 * DesfireImpl.c
 * MIFARE DESFire backend
 *
 *  Created on: 02.11.2016
 *      Author: dev_zzo
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "MifareDesfireBE.h"

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

/*
 * Internal state variables
 */

typedef struct {
    uint8_t Slot;
    uint8_t KeySettings;
    uint8_t KeyCount;
    uint16_t FilesAddress; /* in FRAM */
    uint16_t KeyAddress; /* in FRAM */
    uint32_t DirtyFlags;
} SelectedAppCacheType;

typedef struct {
    uint8_t Num;
    DesfireFileType File;
} SelectedFileCacheType;

static uint16_t CardCapacityBlocks;

/* Cached data: flush to FRAM if changed */
static DesfirePiccInfoType Picc;
static DesfireAppDirType AppDir;

/* Cached app data */
static SelectedAppCacheType SelectedApp;
static SelectedFileCacheType SelectedFile;

typedef void (*TransferSourceFuncType)(uint8_t* Buffer, uint8_t Count);
typedef void (*TransferSinkFuncType)(uint8_t* Buffer, uint8_t Count);
typedef void (*TransferChecksumUpdateFuncType)(const uint8_t* Buffer, uint8_t Count);
typedef uint8_t (*TransferChecksumFinalFuncType)(uint8_t* Buffer);
typedef uint8_t (*TransferEncryptFuncType)(uint8_t* Buffer, uint8_t Count);
typedef TransferStatus (*PiccToPcdTransferFilterFuncType)(uint8_t* Buffer);
typedef uint8_t (*PcdToPiccTransferFilterFuncType)(uint8_t* Buffer, uint8_t Count);

/* Stored transfer state for all transfers */
typedef union {
    struct {
        uint8_t NextIndex;
    } GetApplicationIds;

    struct {
        uint16_t BytesLeft;
        struct {
            TransferSourceFuncType Func;
            uint16_t Pointer; /* in FRAM */
        } Source;
        bool FirstPaddingBitSet;
        struct {
            TransferChecksumUpdateFuncType UpdateFunc;
            TransferChecksumFinalFuncType FinalFunc;
            uint8_t AvailablePlaintext;
            union {
                struct {
                    uint8_t BlockBuffer[CRYPTO_DES_BLOCK_SIZE];
                } MAC;
                uint16_t CRCA;
            };
        } Checksums;
        struct {
            TransferEncryptFuncType Func;
            uint8_t AvailablePlaintext;
            union {
                struct {
                    uint8_t BlockBuffer[CRYPTO_DES_BLOCK_SIZE];
                } TDEA;
            };
        } Encryption;
    } ReadData;

    struct {
        struct {
            TransferSinkFuncType Func;
            uint16_t Pointer; /* in FRAM */
        } Sink;
        struct {
            PcdToPiccTransferFilterFuncType Func;
            uint16_t BytesLeft;
            union {
                struct {
                    uint8_t PartialBlock[8];
                    uint8_t PartialCount;
                } TDEAMAC;
                struct {
                    uint8_t PartialBlock[8];
                    uint8_t PartialCount;
                    uint16_t ChecksumComputed;
                    uint8_t Checksum1stByte;
                    uint8_t RemainingChecksumBytes;
                } TDEAEncrypt;
            } Params;
        } Filter;
    } WriteData;
} TransferStateType;

static TransferStateType TransferState;

static void SynchronizePiccInfo(void);

/*
 * "EEPROM" memory management routines
 */

static void ReadBlockBytes(void* Buffer, uint8_t StartBlock, uint16_t Count)
{
    MemoryReadBlock(Buffer, StartBlock * DESFIRE_EEPROM_BLOCK_SIZE, Count);
}

static void WriteBlockBytes(void* Buffer, uint8_t StartBlock, uint16_t Count)
{
    MemoryWriteBlock(Buffer, StartBlock * DESFIRE_EEPROM_BLOCK_SIZE, Count);
}

static void CopyBlockBytes(uint8_t DestBlock, uint8_t SrcBlock, uint16_t Count)
{
    uint8_t Buffer[DESFIRE_EEPROM_BLOCK_SIZE];
    uint16_t SrcOffset = SrcBlock * DESFIRE_EEPROM_BLOCK_SIZE;
    uint16_t DestOffset = DestBlock * DESFIRE_EEPROM_BLOCK_SIZE;

    while (Count--) {
        MemoryReadBlock(Buffer, SrcOffset, Count);
        MemoryWriteBlock(Buffer, DestOffset, Count);
        SrcOffset += DESFIRE_EEPROM_BLOCK_SIZE;
        DestOffset += DESFIRE_EEPROM_BLOCK_SIZE;
    }
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
    MemorySetBlock(0x00, Block * DESFIRE_EEPROM_BLOCK_SIZE, BlockCount * DESFIRE_EEPROM_BLOCK_SIZE);
    return Block;
}

static uint8_t GetCardCapacityBlocks(void)
{
    uint8_t MaxFreeBlocks;

    switch (Picc.StorageSize) {
    case DESFIRE_STORAGE_SIZE_2K:
        MaxFreeBlocks = 0x40 - DESFIRE_FIRST_FREE_BLOCK_ID;
        break;
    case DESFIRE_STORAGE_SIZE_4K:
        MaxFreeBlocks = 0x80 - DESFIRE_FIRST_FREE_BLOCK_ID;
        break;
    case DESFIRE_STORAGE_SIZE_8K:
        MaxFreeBlocks = 0 - DESFIRE_FIRST_FREE_BLOCK_ID;
        break;
    default:
        return 0;
    }

    return MaxFreeBlocks - Picc.FirstFreeBlock;
}

/*
 * Global card structure support routines
 */

static void SynchronizeAppDir(void)
{
    WriteBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(DesfireAppDirType));
}

static void SynchronizePiccInfo(void)
{
    WriteBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(Picc));
}

static uint8_t GetAppProperty(uint8_t BlockId, uint8_t AppSlot)
{
    DesfireApplicationDataType Data;

    ReadBlockBytes(&Data, BlockId, sizeof(Data));
    return Data.AppData[AppSlot];
}

static void SetAppProperty(uint8_t BlockId, uint8_t AppSlot, uint8_t Value)
{
    DesfireApplicationDataType Data;

    ReadBlockBytes(&Data, BlockId, sizeof(Data));
    Data.AppData[AppSlot] = Value;
    WriteBlockBytes(&Data, BlockId, sizeof(Data));
}

static void SetAppKeyCount(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(DESFIRE_APP_KEY_COUNT_BLOCK_ID, AppSlot, Value);
}

static uint8_t GetAppKeySettings(uint8_t AppSlot)
{
    return GetAppProperty(DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot);
}

static void SetAppKeySettings(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot, Value);
}

static void SetAppKeyStorageBlockId(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(DESFIRE_APP_KEYS_PTR_BLOCK_ID, AppSlot, Value);
}

static uint8_t GetAppFileIndexBlockId(uint8_t AppSlot)
{
    return GetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot);
}

static void SetAppFileIndexBlockId(uint8_t AppSlot, uint8_t Value)
{
    SetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot, Value);
}

/*
 * Application key management
 */

uint8_t GetSelectedAppKeyCount(void)
{
    return SelectedApp.KeyCount;
}

uint8_t GetSelectedAppKeySettings(void)
{
    return SelectedApp.KeySettings;
}

void SetSelectedAppKeySettings(uint8_t KeySettings)
{
    SelectedApp.KeySettings = KeySettings;
    SetAppKeySettings(SelectedApp.Slot, KeySettings);
}

void ReadSelectedAppKey(uint8_t KeyId, uint8_t* Key)
{
    MemoryReadBlock(Key, SelectedApp.KeyAddress + KeyId * sizeof(Desfire2KTDEAKeyType), sizeof(Desfire2KTDEAKeyType));
}

void WriteSelectedAppKey(uint8_t KeyId, const uint8_t* Key)
{
    MemoryWriteBlock(Key, SelectedApp.KeyAddress + KeyId * sizeof(Desfire2KTDEAKeyType), sizeof(Desfire2KTDEAKeyType));
}

/*
 * Application selection
 */

static uint8_t LookupAppSlot(const DesfireAidType Aid)
{
    uint8_t Slot;

    for (Slot = 0; Slot < DESFIRE_MAX_SLOTS; ++Slot) {
        if (!memcmp(AppDir.AppIds[Slot], Aid, DESFIRE_AID_SIZE))
            break;
    }

    return Slot;
}

static void SelectAppBySlot(uint8_t AppSlot)
{
    /* TODO: verify this behaviour */
    AbortTransaction();

    SelectedApp.Slot = AppSlot;

    SelectedApp.KeySettings = GetAppProperty(DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot);
    SelectedApp.KeyCount = GetAppProperty(DESFIRE_APP_KEY_COUNT_BLOCK_ID, AppSlot);
    SelectedApp.KeyAddress = GetAppProperty(DESFIRE_APP_KEYS_PTR_BLOCK_ID, AppSlot) * DESFIRE_EEPROM_BLOCK_SIZE;
    SelectedApp.FilesAddress = GetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot) * DESFIRE_EEPROM_BLOCK_SIZE;
}

uint8_t SelectApp(const DesfireAidType Aid)
{
    uint8_t Slot;

    /* Search for the app slot */
    Slot = LookupAppSlot(Aid);
    if (Slot == DESFIRE_MAX_SLOTS) {
        return STATUS_APP_NOT_FOUND;
    }

    SelectAppBySlot(Slot);
    return STATUS_OPERATION_OK;
}

void SelectPiccApp(void)
{
    SelectAppBySlot(DESFIRE_PICC_APP_SLOT);
}

bool IsPiccAppSelected(void)
{
    return SelectedApp.Slot == DESFIRE_PICC_APP_SLOT;
}

/*
 * Application management
 */

uint8_t CreateApp(const DesfireAidType Aid, uint8_t KeyCount, uint8_t KeySettings)
{
    uint8_t Slot;
    uint8_t FreeSlot;
    uint8_t KeysBlockId, FilesBlockId;

    /* Verify this AID has not been allocated yet */
    if (LookupAppSlot(Aid) != DESFIRE_MAX_SLOTS) {
        return STATUS_DUPLICATE_ERROR;
    }
    /* Verify there is space */
    Slot = AppDir.FirstFreeSlot;
    if (Slot == DESFIRE_MAX_SLOTS) {
        return STATUS_APP_COUNT_ERROR;
    }
    /* Update the next free slot */
    for (FreeSlot = Slot + 1; FreeSlot < DESFIRE_MAX_SLOTS; ++FreeSlot) {
        if ((AppDir.AppIds[FreeSlot][0] | AppDir.AppIds[FreeSlot][1] | AppDir.AppIds[FreeSlot][2]) == 0)
            break;
    }

    /* Allocate storage for the application */
    KeysBlockId = AllocateBlocks(DESFIRE_BYTES_TO_BLOCKS(KeyCount * sizeof(Desfire2KTDEAKeyType)));
    if (!KeysBlockId) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    }
    FilesBlockId = AllocateBlocks(DESFIRE_FILE_INDEX_BLOCKS);
    if (!FilesBlockId) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    }

    /* Initialise the application */
    SetAppKeySettings(Slot, KeySettings);
    SetAppKeyCount(Slot, KeyCount);
    SetAppKeyStorageBlockId(Slot, KeysBlockId);
    SetAppFileIndexBlockId(Slot, FilesBlockId);

    /* Update the directory */
    AppDir.FirstFreeSlot = FreeSlot;
    AppDir.AppIds[Slot][0] = Aid[0];
    AppDir.AppIds[Slot][1] = Aid[1];
    AppDir.AppIds[Slot][2] = Aid[2];
    SynchronizeAppDir();

    return STATUS_OPERATION_OK;
}

uint8_t DeleteApp(const DesfireAidType Aid)
{
    uint8_t Slot;

    /* Search for the app slot */
    Slot = LookupAppSlot(Aid);
    if (Slot == DESFIRE_MAX_SLOTS) {
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

    if (Slot == SelectedApp.Slot) {
        SelectAppBySlot(DESFIRE_PICC_APP_SLOT);
    }
    return STATUS_OPERATION_OK;
}

void GetApplicationIdsSetup(void)
{
    /* Skip the PICC application */
    TransferState.GetApplicationIds.NextIndex = 1;
}

TransferStatus GetApplicationIdsTransfer(uint8_t* Buffer)
{
    TransferStatus Status;
    uint8_t EntryIndex;

    Status.BytesProcessed = 0;
    for (EntryIndex = TransferState.GetApplicationIds.NextIndex; EntryIndex < DESFIRE_MAX_SLOTS; ++EntryIndex) {
        if ((AppDir.AppIds[EntryIndex][0] | AppDir.AppIds[EntryIndex][1] | AppDir.AppIds[EntryIndex][2]) == 0)
            continue;
        /* If won't fit -- remember and return */
        if (Status.BytesProcessed >= DESFIRE_AID_SIZE * 19) {
            TransferState.GetApplicationIds.NextIndex = EntryIndex;
            Status.IsComplete = false;
            return Status;
        }
        Buffer[Status.BytesProcessed++] = AppDir.AppIds[EntryIndex][0];
        Buffer[Status.BytesProcessed++] = AppDir.AppIds[EntryIndex][1];
        Buffer[Status.BytesProcessed++] = AppDir.AppIds[EntryIndex][2];
    }
    Status.IsComplete = true;
    return Status;
}

/*
 * File management: creation, deletion, and misc routines
 */

static uint8_t GetFileControlBlockId(uint8_t FileNum)
{
    uint8_t FileIndexBlock;
    DesfireFileIndexType FileIndex;

    /* Read in the file index */
    FileIndexBlock = GetAppFileIndexBlockId(SelectedApp.Slot);
    ReadBlockBytes(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    return FileIndex[FileNum];
}

static uint8_t GetFileDataAreaBlockId(uint8_t FileNum)
{
    return GetFileControlBlockId(FileNum) + 1;
}

static uint8_t ReadFileControlBlock(uint8_t FileNum, DesfireFileType* File)
{
    uint8_t BlockId;

    /* Check whether the file exists */
    BlockId = GetFileControlBlockId(FileNum);
    if (!BlockId) {
        return STATUS_FILE_NOT_FOUND;
    }
    /* Read the file control block */
    ReadBlockBytes(File, BlockId, sizeof(File));
    return STATUS_OPERATION_OK;
}

static uint8_t WriteFileControlBlock(uint8_t FileNum, DesfireFileType* File)
{
    uint8_t BlockId;

    /* Check whether the file exists */
    BlockId = GetFileControlBlockId(FileNum);
    if (!BlockId) {
        return STATUS_FILE_NOT_FOUND;
    }
    /* Write the file control block */
    WriteBlockBytes(File, BlockId, sizeof(File));
    return STATUS_OPERATION_OK;
}

static uint8_t AllocateFileStorage(uint8_t FileNum, uint8_t BlockCount, uint8_t* BlockIdPtr)
{
    uint8_t FileIndexBlock;
    uint8_t FileIndex[DESFIRE_MAX_FILES];
    uint8_t BlockId;

    /* Read in the file index */
    FileIndexBlock = GetAppFileIndexBlockId(SelectedApp.Slot);
    ReadBlockBytes(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    /* Check if the file already exists */
    if (FileIndex[FileNum]) {
        return STATUS_DUPLICATE_ERROR;
    }
    /* Allocate blocks for the file */
    BlockId = AllocateBlocks(1 + BlockCount);
    if (BlockId) {
        /* Write the file index */
        FileIndex[FileNum] = BlockId;
        WriteBlockBytes(&FileIndex, FileIndexBlock, sizeof(FileIndex));
        /* Write the output value */
        *BlockIdPtr = BlockId;
        return STATUS_OPERATION_OK;
    }
    return STATUS_OUT_OF_EEPROM_ERROR;
}

uint8_t CreateStandardFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize)
{
    uint8_t Status;
    uint8_t BlockId;

    /* Grab storage */
    Status = AllocateFileStorage(FileNum, 1 + DESFIRE_BYTES_TO_BLOCKS(FileSize), &BlockId);
    if (Status == STATUS_OPERATION_OK) {
        /* Fill in the control structure and write it */
        memset(&SelectedFile, 0, sizeof(SelectedFile));
        SelectedFile.File.Type = DESFIRE_FILE_STANDARD_DATA;
        SelectedFile.File.CommSettings = CommSettings;
        SelectedFile.File.AccessRights = AccessRights;
        SelectedFile.File.StandardFile.FileSize = FileSize;
        WriteBlockBytes(&SelectedFile.File, BlockId, sizeof(SelectedFile.File));
    }
    /* Done */
    return Status;
}

uint8_t CreateBackupFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize)
{
    uint8_t Status;
    uint8_t BlockId;
    uint8_t BlockCount;

    /* Grab storage */
    BlockCount = DESFIRE_BYTES_TO_BLOCKS(FileSize);
    Status = AllocateFileStorage(FileNum, 1 + 2 * BlockCount, &BlockId);
    if (Status == STATUS_OPERATION_OK) {
        /* Fill in the control structure and write it */
        memset(&SelectedFile, 0, sizeof(SelectedFile));
        SelectedFile.File.Type = DESFIRE_FILE_BACKUP_DATA;
        SelectedFile.File.CommSettings = CommSettings;
        SelectedFile.File.AccessRights = AccessRights;
        SelectedFile.File.BackupFile.FileSize = FileSize;
        SelectedFile.File.BackupFile.BlockCount = BlockCount;
        WriteBlockBytes(&SelectedFile.File, BlockId, sizeof(SelectedFile.File));
    }
    /* Done */
    return Status;
}

uint8_t CreateValueFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights,
    int32_t LowerLimit, int32_t UpperLimit, int32_t Value, bool LimitedCreditEnabled)
{
    uint8_t Status;
    uint8_t BlockId;

    /* Grab storage */
    Status = AllocateFileStorage(FileNum, 1, &BlockId);
    if (Status == STATUS_OPERATION_OK) {
        /* Fill in the control structure and write it */
        memset(&SelectedFile, 0, sizeof(SelectedFile));
        SelectedFile.File.Type = DESFIRE_FILE_VALUE_DATA;
        SelectedFile.File.CommSettings = CommSettings;
        SelectedFile.File.AccessRights = AccessRights;
        SelectedFile.File.ValueFile.LowerLimit = LowerLimit;
        SelectedFile.File.ValueFile.UpperLimit = UpperLimit;
        SelectedFile.File.ValueFile.CleanValue = Value;
        SelectedFile.File.ValueFile.DirtyValue = Value;
        SelectedFile.File.ValueFile.LimitedCreditEnabled = LimitedCreditEnabled;
        SelectedFile.File.ValueFile.PreviousDebit = 0;
        WriteBlockBytes(&SelectedFile.File, BlockId, sizeof(SelectedFile.File));
    }
    /* Done */
    return Status;
}

uint8_t DeleteFile(uint8_t FileNum)
{
    uint8_t FileIndexBlock;
    uint8_t FileIndex[DESFIRE_MAX_FILES];

    FileIndexBlock = GetAppFileIndexBlockId(SelectedApp.Slot);
    ReadBlockBytes(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    if (FileIndex[FileNum]) {
        FileIndex[FileNum] = 0;
    }
    else {
        return STATUS_FILE_NOT_FOUND;
    }
    WriteBlockBytes(&FileIndex, FileIndexBlock, sizeof(FileIndex));
    return STATUS_OPERATION_OK;
}

/*
 * File management: transaction related routines
 */

static void StartTransaction(void)
{
    Picc.TransactionStarted = SelectedApp.Slot;
    SynchronizePiccInfo();
    SelectedApp.DirtyFlags = 0;
}

static void MarkFileDirty(uint8_t FileNum)
{
    SelectedApp.DirtyFlags |= 1 << FileNum;
}

static void StopTransaction(void)
{
    Picc.TransactionStarted = 0;
    SynchronizePiccInfo();
}

static void SynchFileCopies(uint8_t FileNum, bool Rollback)
{
    DesfireFileType File;
    uint8_t DataAreaBlockId;

    ReadFileControlBlock(FileNum, &File);
    DataAreaBlockId = GetFileDataAreaBlockId(FileNum);

    switch (File.Type) {
    case DESFIRE_FILE_VALUE_DATA:
        if (Rollback) {
            File.ValueFile.DirtyValue = File.ValueFile.CleanValue;
        }
        else {
            File.ValueFile.CleanValue = File.ValueFile.DirtyValue;
        }
        WriteFileControlBlock(FileNum, &File);
        break;

    case DESFIRE_FILE_BACKUP_DATA:
        if (Rollback) {
            CopyBlockBytes(DataAreaBlockId, DataAreaBlockId + File.BackupFile.BlockCount, File.BackupFile.BlockCount);
        }
        else {
            CopyBlockBytes(DataAreaBlockId + File.BackupFile.BlockCount, DataAreaBlockId, File.BackupFile.BlockCount);
        }
        break;
    }
    /* TODO: implement other file types */
}

static void FinaliseTransaction(bool Rollback)
{
    uint8_t FileNum;
    uint16_t DirtyFlags = SelectedApp.DirtyFlags;

    if (!Picc.TransactionStarted) 
        return;
    for (FileNum = 0; FileNum < DESFIRE_MAX_FILES; ++FileNum) {
        if (DirtyFlags & (1 << FileNum)) {
            SynchFileCopies(FileNum, Rollback);
        }
    }
    StopTransaction();
}

void CommitTransaction(void)
{
    FinaliseTransaction(false);
}

void AbortTransaction(void)
{
    FinaliseTransaction(true);
}

/*
 * File management: data transfer related routines
 */

uint8_t SelectFile(uint8_t FileNum)
{
    SelectedFile.Num = FileNum;
    return ReadFileControlBlock(FileNum, &SelectedFile.File);
}

uint8_t GetSelectedFileType(void)
{
    return SelectedFile.File.Type;
}

uint8_t GetSelectedFileCommSettings(void)
{
    return SelectedFile.File.CommSettings;
}

uint16_t GetSelectedFileAccessRights(void)
{
    return SelectedFile.File.AccessRights;
}

static void ReadDataEepromSource(uint8_t* Buffer, uint8_t Count)
{
    MemoryReadBlock(Buffer, TransferState.ReadData.Source.Pointer, Count);
    TransferState.ReadData.Source.Pointer += Count;
}

static void WriteDataEepromSink(uint8_t* Buffer, uint8_t Count)
{
    MemoryWriteBlock(Buffer, TransferState.WriteData.Sink.Pointer, Count);
    TransferState.WriteData.Sink.Pointer += Count;
}

static void PaddingTDEA(uint8_t* Buffer, uint8_t Count, bool FirstPaddingBitSet)
{
    uint8_t PaddingByte = FirstPaddingBitSet ? 0x80 : 0x00;
    uint8_t PaddingSize = CRYPTO_DES_BLOCK_SIZE - (Count & (CRYPTO_DES_BLOCK_SIZE - 1));
    uint8_t i;

    for (i = Count; i < Count + PaddingSize; ++i) {
        Buffer[i] = PaddingByte;
        PaddingByte = 0x00;
    }
}

#define DESFIRE_MAX_PAYLOAD_TDEA_BLOCKS (DESFIRE_MAX_PAYLOAD_SIZE / CRYPTO_DES_BLOCK_SIZE)

static void PiccToPcdUpdateCRCA(const uint8_t* Buffer, uint8_t Count)
{
    TransferState.ReadData.Checksums.CRCA = ISO14443AUpdateCRCA(Buffer, Count, TransferState.ReadData.Checksums.CRCA);
}

static uint8_t PiccToPcdFinalCRCA(uint8_t* Buffer)
{
    /* Copy the checksum to destination */
    memcpy(Buffer, &TransferState.ReadData.Checksums.CRCA, 2);
    /* Return the checksum size */
    return 2;
}

static void PiccToPcdUpdateMACTDEASend(const uint8_t* Buffer, uint8_t Count)
{
    uint8_t AvailablePlaintext = TransferState.ReadData.Checksums.AvailablePlaintext;
    uint8_t TempBytes;
    uint8_t TempBuffer[CRYPTO_DES_BLOCK_SIZE];

    if (AvailablePlaintext) {
        /* Fill the partial block */
        TempBytes = CRYPTO_DES_BLOCK_SIZE - AvailablePlaintext;
        if (TempBytes > Count)
            TempBytes = Count;
        memcpy(&TransferState.ReadData.Checksums.MAC.BlockBuffer[AvailablePlaintext], &Buffer[0], TempBytes);
        Count -= TempBytes;
        Buffer += TempBytes;
        /* MAC the partial block */
        CryptoEncrypt2KTDEA_CBCSend(1, &TransferState.ReadData.Checksums.MAC.BlockBuffer[0], &TempBuffer[0], SessionIV, SessionKey);
    }
    /* MAC complete blocks in the buffer */
    while (Count >= CRYPTO_DES_BLOCK_SIZE) {
        /* NOTE: This is block-by-block, hence slow. See if it's better to just allocate a temp buffer large enough (64 bytes). */
        CryptoEncrypt2KTDEA_CBCSend(1, &Buffer[0], &TempBuffer[0], SessionIV, SessionKey);
        Count -= CRYPTO_DES_BLOCK_SIZE;
        Buffer += CRYPTO_DES_BLOCK_SIZE;
    }
    /* Copy the new partial block */
    if (Count) {
        memcpy(&TransferState.ReadData.Checksums.MAC.BlockBuffer[0], &Buffer[0], Count);
    }
    TransferState.ReadData.Checksums.AvailablePlaintext = Count;
}

static uint8_t PiccToPcdFinalMACTDEASend(uint8_t* Buffer)
{
    uint8_t AvailablePlaintext = TransferState.ReadData.Checksums.AvailablePlaintext;
    uint8_t TempBuffer[CRYPTO_DES_BLOCK_SIZE];

    if (AvailablePlaintext) {
        /* Apply padding */
        PaddingTDEA(&TransferState.ReadData.Checksums.MAC.BlockBuffer[0], AvailablePlaintext, TransferState.ReadData.FirstPaddingBitSet);
        /* MAC the partial block */
        CryptoEncrypt2KTDEA_CBCSend(1, &TransferState.ReadData.Checksums.MAC.BlockBuffer[0], &TempBuffer[0], SessionIV, SessionKey);
        TransferState.ReadData.Checksums.AvailablePlaintext = 0;
    }
    /* Copy the checksum to destination */
    memcpy(Buffer, SessionIV, 4);
    /* Return the checksum size */
    return 4;
}

static uint8_t PiccToPcdEncryptTDEASend(uint8_t* Buffer, uint8_t Count)
{
    uint8_t AvailablePlaintext = TransferState.ReadData.Encryption.AvailablePlaintext;
    uint8_t TempBuffer[(DESFIRE_MAX_PAYLOAD_TDEA_BLOCKS + 1) * CRYPTO_DES_BLOCK_SIZE];
    uint8_t BlockCount;

    if (AvailablePlaintext) {
        /* Fill the partial block */
        memcpy(&TempBuffer[0], &TransferState.ReadData.Encryption.TDEA.BlockBuffer[0], AvailablePlaintext);
    }
    /* Copy fresh plaintext to the temp buffer */
    memcpy(&TempBuffer[AvailablePlaintext], Buffer, Count);
    Count += AvailablePlaintext;
    BlockCount = Count / CRYPTO_DES_BLOCK_SIZE;
    /* Stash extra plaintext for later */
    AvailablePlaintext = Count - BlockCount * CRYPTO_DES_BLOCK_SIZE;
    if (AvailablePlaintext) {
        memcpy(&TransferState.ReadData.Encryption.TDEA.BlockBuffer[0], &Buffer[BlockCount * CRYPTO_DES_BLOCK_SIZE], AvailablePlaintext);
    }
    TransferState.ReadData.Encryption.AvailablePlaintext = AvailablePlaintext;
    /* Encrypt complete blocks in the buffer */
    CryptoEncrypt2KTDEA_CBCSend(BlockCount, &TempBuffer[0], &Buffer[0], SessionIV, SessionKey);
    /* Return byte count to transfer */
    return BlockCount * CRYPTO_DES_BLOCK_SIZE;
}

static TransferStatus PiccToPcdTransfer(uint8_t* Buffer)
{
    TransferStatus Status;
    uint8_t XferBytes;

    /* Only read if required */
    if (TransferState.ReadData.BytesLeft) {
        /* Figure out how much to read */
        XferBytes = (uint8_t)TransferState.ReadData.BytesLeft;
        if (TransferState.ReadData.BytesLeft > DESFIRE_MAX_PAYLOAD_TDEA_BLOCKS * CRYPTO_DES_BLOCK_SIZE) {
            XferBytes = DESFIRE_MAX_PAYLOAD_TDEA_BLOCKS * CRYPTO_DES_BLOCK_SIZE;
        }
        /* Read input bytes */
        TransferState.ReadData.Source.Func(Buffer, XferBytes);
        TransferState.ReadData.BytesLeft -= XferBytes;
        /* Update checksum/MAC */
        if (TransferState.ReadData.Checksums.UpdateFunc)
            TransferState.ReadData.Checksums.UpdateFunc(Buffer, XferBytes);
        if (TransferState.ReadData.BytesLeft == 0) {
            /* Finalise checksumming and append the checksum */
            if (TransferState.ReadData.Checksums.FinalFunc)
                XferBytes += TransferState.ReadData.Checksums.FinalFunc(&Buffer[XferBytes]);
        }
        /* Encrypt */
        Status.BytesProcessed = TransferState.ReadData.Encryption.Func(Buffer, XferBytes);
        Status.IsComplete = TransferState.ReadData.Encryption.AvailablePlaintext == 0;
    }
    else {
        /* Final encryption block */
        Status.BytesProcessed = TransferState.ReadData.Encryption.Func(Buffer, 0);
        Status.IsComplete = true;
    }

    return Status;
}


#if 0
static uint8_t PcdToPiccTransferFilterNull(uint8_t* Buffer, uint8_t ByteCount)
{
    TransferStatus Status;

    Status = TransferState.WriteData.Sink.Func(Buffer, ByteCount);
    if (!Status.IsComplete) {
        return STATUS_ADDITIONAL_FRAME;
    }
    if (Status.BytesProcessed == ByteCount) {
        return STATUS_OPERATION_OK;
    }
    /* TODO: Verify this behaviour */
    return STATUS_BOUNDARY_ERROR;
}

static uint8_t PrependPartialDataMAC(uint8_t* Buffer, uint8_t ByteCount, uint8_t PartialCount)
{
    if (PartialCount) {
        memmove(&Buffer[PartialCount], &Buffer[0], ByteCount);
        memcpy(&Buffer[0], &TransferState.WriteData.Filter.Params.TDEAMAC.PartialBlock[0], PartialCount);
        ByteCount += PartialCount;
    }
    TransferState.WriteData.Filter.Params.TDEAMAC.PartialCount = 0;
    return ByteCount;
}

static uint8_t PcdToPiccTransferFilterMAC(uint8_t* Buffer, uint8_t ByteCount)
{
    TransferStatus Status;
    uint8_t PartialCount = TransferState.WriteData.Filter.Params.TDEAMAC.PartialCount;

    /* Prepend previously received leftover data */
    ByteCount = PrependPartialDataMAC(Buffer, ByteCount, PartialCount);
    /* Compute how many complete blocks we have; stash extra if any */
    XferBlocks = ByteCount / CRYPTO_DES_BLOCK_SIZE;
    XferBytes = XferBlocks * CRYPTO_DES_BLOCK_SIZE;
    PartialCount = ByteCount - XferBytes;
    if (PartialCount) {
        memcpy(&TransferState.WriteData.Filter.Params.TDEAMAC.PartialBlock[0], &Buffer[XferBytes], PartialCount);
        TransferState.WriteData.Filter.Params.TDEAMAC.PartialCount = PartialCount;
    }
    /* Push plaintext data to the sink; it will write no more than required */
    Status = TransferState.WriteData.Sink.Func(Buffer, XferBytes);
    /* Update running MAC over complete blocks */
    CryptoEncrypt2KTDEA_CBCReceive(1, Buffer, Buffer, SessionIV, SessionKey);

    /* If we pushed all the bytes we had to, it is time to verify MAC */
    if (Status.IsComplete) {
        /* Received MAC will be stashed in PartialBlock[] since it is shorter than one DES block */
    }

    /* TODO */
    return STATUS_PICC_INTEGRITY_ERROR;
}

static uint8_t PrependPartialDataEncrypt(uint8_t* Buffer, uint8_t ByteCount, uint8_t PartialCount)
{
    if (PartialCount) {
        memmove(&Buffer[PartialCount], &Buffer[0], ByteCount);
        memcpy(&Buffer[0], &TransferState.WriteData.Filter.Params.TDEAEncrypt.PartialBlock[0], PartialCount);
        ByteCount += PartialCount;
    }
    TransferState.WriteData.Filter.Params.TDEAEncrypt.PartialCount = 0;
    return ByteCount;
}

static uint8_t PcdToPiccTransferFilterEncryptFinal(uint8_t* Buffer, uint8_t ByteCount)
{
    uint8_t PartialCount = TransferState.WriteData.Filter.Params.TDEAEncrypt.PartialCount;
    uint16_t ChecksumReceived;

    /* Prepend previously received leftover data */
    ByteCount = PrependPartialDataEncrypt(Buffer, ByteCount, PartialCount);
    /* TODO: Verify we've got a complete block */
    /* Encrypt the complete block in CBC receive mode to recover plaintext */
    CryptoEncrypt2KTDEA_CBCReceive(1, Buffer, Buffer, SessionIV, SessionKey);
    /* Depending on how many bytes we expected, build the checksum */
    if (TransferState.WriteData.Filter.Params.TDEAEncrypt.RemainingChecksumBytes == 2) {
        ChecksumReceived = *(uint16_t*)&Buffer[0];
    }
    else {
        ChecksumReceived = (Buffer[0] << 8) | TransferState.WriteData.Filter.Params.TDEAEncrypt.Checksum1stByte;
    }
    /* Verify checksum */
    if (TransferState.WriteData.Filter.Params.TDEAEncrypt.ChecksumComputed == ChecksumReceived) {
        return STATUS_OPERATION_OK;
    }
    else {
        return STATUS_PICC_INTEGRITY_ERROR;
    }
}

static uint8_t PcdToPiccTransferFilterEncrypt(uint8_t* Buffer, uint8_t ByteCount)
{
    TransferStatus Status;
    uint8_t PartialCount = TransferState.WriteData.Filter.Params.TDEAEncrypt.PartialCount;
    uint8_t XferBlocks;
    uint8_t XferBytes;

    /* Prepend previously received leftover data */
    ByteCount = PrependPartialDataEncrypt(Buffer, ByteCount, PartialCount);
    /* Compute how many complete blocks we have; stash extra if any */
    XferBlocks = ByteCount / CRYPTO_DES_BLOCK_SIZE;
    XferBytes = XferBlocks * CRYPTO_DES_BLOCK_SIZE;
    PartialCount = ByteCount - XferBytes;
    if (PartialCount) {
        memcpy(&TransferState.WriteData.Filter.Params.TDEAEncrypt.PartialBlock[0], &Buffer[XferBytes], PartialCount);
        TransferState.WriteData.Filter.Params.TDEAEncrypt.PartialCount = PartialCount;
    }
    /* Encrypt complete blocks in CBC receive mode to recover plaintext */
    CryptoEncrypt2KTDEA_CBCReceive(XferBlocks, Buffer, Buffer, SessionIV, SessionKey);
    /* Push plaintext data to the sink; it will write no more than required */
    Status = TransferState.WriteData.Sink.Func(Buffer, XferBytes);
    /* Update running checksum over written bytes */
    TransferState.WriteData.Filter.Params.TDEAEncrypt.ChecksumComputed = ISO14443AUpdateCRCA(Buffer, Status.BytesProcessed, TransferState.WriteData.Filter.Params.TDEAEncrypt.ChecksumComputed);
    /* If we pushed all the bytes we had to, it is time to verify checksum */
    if (Status.IsComplete) {
        /* Compute how many plaintext bytes we have remaining */
        uint8_t ExtraBytes = XferBytes - Status.BytesProcessed;
        /* Checksum can be in the same block. */
        switch (ExtraBytes) {
        case 0:
            /* The checksum is completely in the next block */
            TransferState.WriteData.Filter.Params.TDEAEncrypt.RemainingChecksumBytes = 2;
            TransferState.WriteData.Filter.Func = &PcdToPiccTransferFilterEncryptFinal;
            return STATUS_ADDITIONAL_FRAME;

        case 1:
            /* The checksum is split between this and the next block */
            TransferState.WriteData.Filter.Params.TDEAEncrypt.Checksum1stByte = Buffer[Status.BytesProcessed];
            TransferState.WriteData.Filter.Params.TDEAEncrypt.RemainingChecksumBytes = 1;
            TransferState.WriteData.Filter.Func = &PcdToPiccTransferFilterEncryptFinal;
            return STATUS_ADDITIONAL_FRAME;

        default:
            /* TODO: check padding */
        case 2:
            /* Have >= 2; checksum must be at &Buffer[Status.BytesProcessed] */
            if (TransferState.WriteData.Filter.Params.TDEAEncrypt.ChecksumComputed == *(uint16_t*)&Buffer[Status.BytesProcessed]) {
                return STATUS_OPERATION_OK;
            }
            else {
                return STATUS_PICC_INTEGRITY_ERROR;
            }
        }
    }

    return STATUS_ADDITIONAL_FRAME;
}
#endif

static uint8_t ReadDataFilterSetup(uint8_t CommSettings)
{
    memset(&TransferState, 0, sizeof(TransferState));

    switch (CommSettings) {
    case DESFIRE_COMMS_PLAINTEXT:
        break;

    case DESFIRE_COMMS_PLAINTEXT_MAC:
        TransferState.ReadData.Checksums.UpdateFunc = &PiccToPcdUpdateMACTDEASend;
        TransferState.ReadData.Checksums.FinalFunc = &PiccToPcdFinalMACTDEASend;
        memset(SessionIV, 0, sizeof(SessionIV));
        break;

    case DESFIRE_COMMS_CIPHERTEXT_DES:
        TransferState.ReadData.Checksums.UpdateFunc = &PiccToPcdUpdateCRCA;
        TransferState.ReadData.Checksums.FinalFunc = &PiccToPcdFinalCRCA;
        TransferState.ReadData.Checksums.CRCA = ISO14443A_CRCA_INIT;
        TransferState.ReadData.Encryption.Func = &PiccToPcdEncryptTDEASend;
        memset(SessionIV, 0, sizeof(SessionIV));
        break;

    default:
        return STATUS_PARAMETER_ERROR;
    }
    return STATUS_OPERATION_OK;
}

static uint8_t WriteDataFilterSetup(uint8_t CommSettings)
{
#if 0
    switch (CommSettings) {
    case DESFIRE_COMMS_PLAINTEXT:
        TransferState.WriteData.Filter.Func = &PcdToPiccTransferFilterNull;
        break;

    case DESFIRE_COMMS_PLAINTEXT_MAC:
        TransferState.WriteData.Filter.Func = &PcdToPiccTransferFilterMAC;
        TransferState.WriteData.Filter.Params.TDEAMAC.PartialCount = 0;
        memset(SessionIV, 0, sizeof(SessionIV));
        break;

    case DESFIRE_COMMS_CIPHERTEXT_DES:
        TransferState.WriteData.Filter.Func = &PcdToPiccTransferFilterEncrypt;
        TransferState.WriteData.Filter.Params.TDEAEncrypt.PartialCount = 0;
        TransferState.WriteData.Filter.Params.TDEAEncrypt.ChecksumComputed = ISO14443A_CRCA_INIT;
        memset(SessionIV, 0, sizeof(SessionIV));
        break;

    default:
        return STATUS_PARAMETER_ERROR;
    }
#endif
    return STATUS_OPERATION_OK;
}

TransferStatus ReadDataFileTransfer(uint8_t* Buffer)
{
    return PiccToPcdTransfer(Buffer);
}

uint8_t WriteDataFileTransfer(uint8_t* Buffer, uint8_t ByteCount)
{
    return TransferState.WriteData.Filter.Func(Buffer, ByteCount);
}

uint8_t ReadDataFileSetup(uint8_t CommSettings, uint16_t Offset, uint16_t Length)
{
    /* Verify boundary conditions */
    if (Offset + Length >= SelectedFile.File.StandardFile.FileSize) {
        return STATUS_BOUNDARY_ERROR;
    }

    /* Setup data source */
    TransferState.ReadData.Source.Func = &ReadDataEepromSource;
    if (!Length) {
        TransferState.ReadData.FirstPaddingBitSet = true;
        TransferState.ReadData.BytesLeft = SelectedFile.File.StandardFile.FileSize - Offset;
    }
    else {
        TransferState.ReadData.FirstPaddingBitSet = false;
        TransferState.ReadData.BytesLeft = Length;
    }
    /* Clean data is always located in the beginning of data area */
    TransferState.ReadData.Source.Pointer = GetFileDataAreaBlockId(SelectedFile.Num) * DESFIRE_EEPROM_BLOCK_SIZE + Offset;
    /* Setup data filter */
    return ReadDataFilterSetup(CommSettings);
}

uint8_t WriteDataFileSetup(uint8_t CommSettings, uint16_t Offset, uint16_t Length)
{
#if 0
    /* Verify boundary conditions */
    if (Offset + Length >= SelectedFile.File.StandardFile.FileSize) {
        return STATUS_BOUNDARY_ERROR;
    }

    /* Setup data sink */
    TransferState.WriteData.Sink.Func = &WriteDataEepromSink;
    TransferState.WriteData.Sink.BytesLeft = Length;
    TransferState.WriteData.Sink.Pointer = GetFileDataAreaBlockId(SelectedFile.Num) * DESFIRE_EEPROM_BLOCK_SIZE + Offset;
    /* Dirty data location depends on the file type */
    if (GetSelectedFileType() == DESFIRE_FILE_BACKUP_DATA) {
        TransferState.WriteData.Sink.Pointer += SelectedFile.File.BackupFile.BlockCount * DESFIRE_EEPROM_BLOCK_SIZE;
    }
    /* Setup data filter */
    return WriteDataFilterSetup(CommSettings);
#endif
}


TransferStatus ReadValueFileTransfer(uint8_t* Buffer)
{
    return PiccToPcdTransfer(Buffer);
}

uint8_t ReadValueFileSetup(uint8_t CommSettings)
{
    /* Setup data source (generic EEPROM source) */
    TransferState.ReadData.Source.Func = &ReadDataEepromSource;
    TransferState.ReadData.Source.Pointer = GetFileDataAreaBlockId(SelectedFile.Num) * DESFIRE_EEPROM_BLOCK_SIZE;
    TransferState.ReadData.BytesLeft = 4;
    TransferState.ReadData.FirstPaddingBitSet = false;
    /* Setup data filter */
    return ReadDataFilterSetup(CommSettings);
}

/*
 * PICC management routines
 */

void RunTDEATests(void);

void InitialisePiccBackendEV0(uint8_t StorageSize)
{
    RunTDEATests();
    /* Init backend junk */
    ReadBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(Picc));
    if (Picc.Uid[0] == 0xFF && Picc.Uid[1] == 0xFF && Picc.Uid[2] == 0xFF && Picc.Uid[3] == 0xFF) {
        DEBUG_PRINT("\r\nFactory resetting the device\r\n");
        FactoryFormatPiccEV0();
    }
    else {
        ReadBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(AppDir));
    }
}

void InitialisePiccBackendEV1(uint8_t StorageSize)
{
    RunTDEATests();
    /* Init backend junk */
    ReadBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(Picc));
    if (Picc.Uid[0] == 0xFF && Picc.Uid[1] == 0xFF && Picc.Uid[2] == 0xFF && Picc.Uid[3] == 0xFF) {
        DEBUG_PRINT("\r\nFactory resetting the device\r\n");
        FactoryFormatPiccEV1(StorageSize);
    }
    else {
        ReadBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(AppDir));
    }
}

void ResetPiccBackend(void)
{
    SelectPiccApp();
}

bool IsEmulatingEV1(void)
{
    return Picc.HwVersionMajor >= DESFIRE_HW_MAJOR_EV1;
}

void GetPiccHardwareVersionInfo(uint8_t* Buffer)
{
    Buffer[0] = Picc.HwVersionMajor;
    Buffer[1] = Picc.HwVersionMinor;
    Buffer[2] = Picc.StorageSize;
}

void GetPiccSoftwareVersionInfo(uint8_t* Buffer)
{
    Buffer[0] = Picc.SwVersionMajor;
    Buffer[1] = Picc.SwVersionMinor;
    Buffer[2] = Picc.StorageSize;
}

void GetPiccManufactureInfo(uint8_t* Buffer)
{
    /* UID/Serial number; does not depend on card mode */
    memcpy(&Buffer[0], &Picc.Uid[0], DESFIRE_UID_SIZE);
    Buffer[7] = Picc.BatchNumber[0];
    Buffer[8] = Picc.BatchNumber[1];
    Buffer[9] = Picc.BatchNumber[2];
    Buffer[10] = Picc.BatchNumber[3];
    Buffer[11] = Picc.BatchNumber[4];
    Buffer[12] = Picc.ProductionWeek;
    Buffer[13] = Picc.ProductionYear;
}

uint8_t GetPiccKeySettings(void)
{
    return GetAppKeySettings(DESFIRE_PICC_APP_SLOT);
}

void FormatPicc(void)
{
    /* Wipe application directory */
    memset(&AppDir, 0, sizeof(AppDir));
    /* Set the first free slot to 1 -- slot 0 is the PICC app */
    AppDir.FirstFreeSlot = 1;
    /* Reset the free block pointer */
    Picc.FirstFreeBlock = DESFIRE_FIRST_FREE_BLOCK_ID;

    SynchronizePiccInfo();
    SynchronizeAppDir();
}

static void CreatePiccApp(void)
{
    Desfire2KTDEAKeyType Key;

    SetAppKeySettings(DESFIRE_PICC_APP_SLOT, 0x0F);
    SetAppKeyCount(DESFIRE_PICC_APP_SLOT, 1);
    SetAppKeyStorageBlockId(DESFIRE_PICC_APP_SLOT, AllocateBlocks(1));
    SelectPiccApp();
    memset(Key, 0, sizeof(Key));
    WriteSelectedAppKey(0, Key);
}

void FactoryFormatPiccEV0(void)
{
    /* Wipe PICC data */
    memset(&Picc, 0xFF, sizeof(Picc));
    memset(&Picc.Uid[0], 0x00, DESFIRE_UID_SIZE);
    /* Initialize params to look like EV0 */
    Picc.StorageSize = DESFIRE_STORAGE_SIZE_4K;
    Picc.HwVersionMajor = DESFIRE_HW_MAJOR_EV0;
    Picc.HwVersionMinor = DESFIRE_HW_MINOR_EV0;
    Picc.SwVersionMajor = DESFIRE_SW_MAJOR_EV0;
    Picc.SwVersionMinor = DESFIRE_SW_MINOR_EV0;
    /* Initialize the root app data */
    CreatePiccApp();
    /* Continue with user data initialization */
    FormatPicc();
}

void FactoryFormatPiccEV1(uint8_t StorageSize)
{
    /* Wipe PICC data */
    memset(&Picc, 0xFF, sizeof(Picc));
    memset(&Picc.Uid[0], 0x00, DESFIRE_UID_SIZE);
    /* Initialize params to look like EV1 */
    Picc.StorageSize = StorageSize;
    Picc.HwVersionMajor = DESFIRE_HW_MAJOR_EV1;
    Picc.HwVersionMinor = DESFIRE_HW_MINOR_EV1;
    Picc.SwVersionMajor = DESFIRE_SW_MAJOR_EV1;
    Picc.SwVersionMinor = DESFIRE_SW_MINOR_EV1;
    /* Initialize the root app data */
    CreatePiccApp();
    /* Continue with user data initialization */
    FormatPicc();
}

void GetPiccUid(ConfigurationUidType Uid)
{
    memcpy(Uid, Picc.Uid, DESFIRE_UID_SIZE);
}

void SetPiccUid(ConfigurationUidType Uid)
{
    memcpy(Picc.Uid, Uid, DESFIRE_UID_SIZE);
    SynchronizePiccInfo();
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
