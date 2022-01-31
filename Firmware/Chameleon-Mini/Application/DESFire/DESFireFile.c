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
 * DESFireFile.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../../Common.h"
#include "../MifareDESFire.h"

#include "DESFireFile.h"
#include "DESFirePICCControl.h"
#include "DESFireStatusCodes.h"
#include "DESFireMemoryOperations.h"
#include "DESFireInstructions.h"
#include "DESFireApplicationDirectory.h"

uint16_t GetFileSizeFromFileType(DESFireFileTypeSettings *File) {
    if (File == NULL) {
        return 0x0000;
    }
    switch (File->FileType) {
        case DESFIRE_FILE_STANDARD_DATA:
            return File->StandardFile.FileSize;
        case DESFIRE_FILE_BACKUP_DATA:
            return File->BackupFile.FileSize;
        case DESFIRE_FILE_VALUE_DATA:
            return sizeof(int32_t); // 4
        case DESFIRE_FILE_LINEAR_RECORDS:
            return (File->RecordFile.BlockCount) * DESFIRE_EEPROM_BLOCK_SIZE;
        case DESFIRE_FILE_CIRCULAR_RECORDS:
        default:
            break;
    }
    return 0x0000;
}

/*
 * File management: creation, deletion, and misc routines
 */

uint16_t GetFileDataAreaBlockId(uint8_t fileIndex) {
    SIZET fileStructAddr = ReadFileDataStructAddress(SelectedApp.Slot, fileIndex);
    if (fileStructAddr == 0) {
        return 0;
    }
    DESFireFileTypeSettings FileData;
    MemoryReadBlock(&FileData, fileStructAddr, sizeof(DESFireFileTypeSettings));
    return FileData.FileDataAddress;
}

uint8_t ReadFileControlBlockIntoCacheStructure(uint8_t FileNum, SelectedFileCacheType *FileCache) {
    if (FileCache == NULL) {
        return STATUS_PARAMETER_ERROR;
    }
    FileCache->Num = FileNum;
    return ReadFileControlBlock(FileNum, &(FileCache->File));
}

uint8_t ReadFileControlBlock(uint8_t FileNum, DESFireFileTypeSettings *File) {
    if (File == NULL) {
        return STATUS_PARAMETER_ERROR;
    }
    uint8_t FileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (FileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_FILE_NOT_FOUND;
    }
    DESFireFileTypeSettings fileSettings = ReadFileSettings(SelectedApp.Slot, FileIndex);
    memcpy(File, &fileSettings, sizeof(DESFireFileTypeSettings));
    File->FileNumber = FileNum;
    return STATUS_OPERATION_OK;
}

uint8_t WriteFileControlBlock(uint8_t FileNum, DESFireFileTypeSettings *File) {
    if (File == NULL) {
        return STATUS_PARAMETER_ERROR;
    }
    uint8_t FileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (FileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_FILE_NOT_FOUND;
    }
    WriteFileSettings(SelectedApp.Slot, FileIndex, File);
    return STATUS_OPERATION_OK;
}

uint8_t CreateFileHeaderData(uint8_t FileNum, uint8_t CommSettings,
                             uint16_t AccessRights, DESFireFileTypeSettings *File) {
    if (File == NULL) {
        return STATUS_PARAMETER_ERROR;
    } else if (SelectedApp.FileCount >= DESFIRE_MAX_FILES) {
        return STATUS_APP_COUNT_ERROR;
    }
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex < DESFIRE_MAX_FILES) {
        return STATUS_DUPLICATE_ERROR;
    }
    fileIndex = LookupNextFreeFileSlot(SelectedApp.Slot);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_APP_COUNT_ERROR;
    }
    SIZET fileSettingsBlockId = AllocateBlocks(DESFIRE_BYTES_TO_BLOCKS(sizeof(DESFireFileTypeSettings)));
    if (fileSettingsBlockId == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        if (File->FileSize > 0) {
            File->FileDataAddress = AllocateBlocks(DESFIRE_BYTES_TO_BLOCKS(File->FileSize));
            if (File->FileDataAddress == 0) {
                return STATUS_OUT_OF_EEPROM_ERROR;
            }
            // The file data is considered to be uninitialized until the user sets it.
            //MemsetBlockBytes(0x00, File->FileDataAddress, File->FileSize);
        } else {
            File->FileDataAddress = 0;
        }
        WriteBlockBytes(File, fileSettingsBlockId, sizeof(DESFireFileTypeSettings));
        SIZET fileAddressArrayBlockId = GetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, SelectedApp.Slot);
        SIZET fileAddressArray[DESFIRE_MAX_FILES];
        ReadBlockBytes(fileAddressArray, fileAddressArrayBlockId, 2 * DESFIRE_MAX_FILES);
        fileAddressArray[fileIndex] = fileSettingsBlockId;
        WriteBlockBytes(fileAddressArray, fileAddressArrayBlockId, 2 * DESFIRE_MAX_FILES);
    }
    WriteFileNumberAtIndex(SelectedApp.Slot, fileIndex, FileNum);
    WriteFileCommSettings(SelectedApp.Slot, fileIndex, CommSettings);
    WriteFileAccessRights(SelectedApp.Slot, fileIndex, AccessRights);
    BYTE nextFileCount = ++(SelectedApp.FileCount);
    WriteFileCount(SelectedApp.Slot, nextFileCount);
    SynchronizeAppDir();
    return STATUS_OPERATION_OK;
}

uint8_t CreateStandardFile(uint8_t FileNum, uint8_t CommSettings,
                           uint16_t AccessRights, uint16_t FileSize) {
    uint8_t Status;
    memset(&SelectedFile, PICC_EMPTY_BYTE, sizeof(SelectedFile));
    SelectedFile.File.FileType = DESFIRE_FILE_STANDARD_DATA;
    SelectedFile.File.StandardFile.FileSize = FileSize;
    SelectedFile.File.FileSize = FileSize;
    Status = CreateFileHeaderData(FileNum, CommSettings, AccessRights, &(SelectedFile.File));
    if (Status != STATUS_OPERATION_OK) {
        return Status;
    }
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_FILE_NOT_FOUND;
    }
    return STATUS_OPERATION_OK;
}

uint8_t CreateBackupFile(uint8_t FileNum, uint8_t CommSettings,
                         uint16_t AccessRights, uint16_t FileSize) {
    uint8_t Status;
    uint8_t BlockCount = DESFIRE_BYTES_TO_BLOCKS(FileSize);
    memset(&SelectedFile, PICC_EMPTY_BYTE, sizeof(SelectedFileCacheType));
    SelectedFile.File.FileType = DESFIRE_FILE_BACKUP_DATA;
    SelectedFile.File.BackupFile.FileSize = FileSize;
    SelectedFile.File.BackupFile.BlockCount = BlockCount;
    SelectedFile.File.FileSize = FileSize;
    Status = CreateFileHeaderData(FileNum, CommSettings, AccessRights, &(SelectedFile.File));
    if (Status != STATUS_OPERATION_OK) {
        return Status;
    }
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_FILE_NOT_FOUND;
    }
    return STATUS_OPERATION_OK;
}

uint8_t CreateValueFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights,
                        int32_t LowerLimit, int32_t UpperLimit, int32_t Value,
                        bool LimitedCreditEnabled) {
    uint8_t Status;
    memset(&SelectedFile, PICC_EMPTY_BYTE, sizeof(SelectedFileCacheType));
    SelectedFile.File.FileType = DESFIRE_FILE_VALUE_DATA;
    SelectedFile.File.ValueFile.LowerLimit = LowerLimit;
    SelectedFile.File.ValueFile.UpperLimit = UpperLimit;
    SelectedFile.File.ValueFile.CleanValue = Value;
    SelectedFile.File.ValueFile.DirtyValue = Value;
    SelectedFile.File.ValueFile.LimitedCreditEnabled = LimitedCreditEnabled;
    SelectedFile.File.ValueFile.PreviousDebit = 0;
    SelectedFile.File.FileSize = 0;
    Status = CreateFileHeaderData(FileNum, CommSettings, AccessRights, &(SelectedFile.File));
    if (Status != STATUS_OPERATION_OK) {
        return Status;
    }
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_FILE_NOT_FOUND;
    }
    return STATUS_OPERATION_OK;
}

uint8_t CreateRecordFile(uint8_t FileType, uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights,
                         uint8_t *RecordSize, uint8_t *MaxRecordSize) {
    uint8_t Status;
    memset(&SelectedFile, PICC_EMPTY_BYTE, sizeof(SelectedFileCacheType));
    SelectedFile.File.FileType = FileType;
    SelectedFile.File.RecordFile.BlockCount = 0;
    SelectedFile.File.RecordFile.RecordPointer = 0;
    memcpy(SelectedFile.File.RecordFile.RecordSize, RecordSize, 3);
    memcpy(SelectedFile.File.RecordFile.MaxRecordCount, MaxRecordSize, 3);
    uint16_t maxRecords = MaxRecordSize[0] | (MaxRecordSize[1] << 8);
    SelectedFile.File.FileSize = maxRecords;
    Status = CreateFileHeaderData(FileNum, CommSettings, AccessRights, &(SelectedFile.File));
    if (Status != STATUS_OPERATION_OK) {
        return Status;
    }
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_FILE_NOT_FOUND;
    }
    return STATUS_OPERATION_OK;
}

uint8_t DeleteFile(uint8_t fileIndex) {
    if (fileIndex >= DESFIRE_MAX_FILES) {
        return STATUS_FILE_NOT_FOUND;
    } else if (SelectedApp.FileCount == 0x00) {
        return STATUS_APP_COUNT_ERROR;
    }
    WriteFileNumberAtIndex(SelectedApp.Slot, fileIndex, DESFIRE_FILE_NOFILE_INDEX);
    WriteFileCommSettings(SelectedApp.Slot, fileIndex, 0x00);
    WriteFileAccessRights(SelectedApp.Slot, fileIndex, 0xffff);
    SIZET fileAddressArray[DESFIRE_MAX_FILES];
    SIZET fileAddressBlockId = GetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, SelectedApp.Slot);
    ReadBlockBytes(fileAddressArray, fileAddressBlockId, 2 * DESFIRE_MAX_FILES);
    fileAddressArray[fileIndex] = 0;
    WriteBlockBytes(fileAddressArray, fileAddressBlockId, 2 * DESFIRE_MAX_FILES);
    WriteFileCount(SelectedApp.Slot, --(SelectedApp.FileCount));
    return STATUS_OPERATION_OK;
}

/* Exposed transfer API: standard/backup data files */

TransferStatus ReadDataFileTransfer(uint8_t *Buffer) {
    return PiccToPcdTransfer(Buffer);
}

uint8_t WriteDataFileTransfer(uint8_t *Buffer, uint8_t ByteCount) {
    return PcdToPiccTransfer(Buffer, ByteCount);
}

uint8_t ReadDataFileSetup(uint8_t FileIndex, uint8_t CommSettings, uint16_t Offset, uint16_t Length) {
    memset(&TransferState, PICC_EMPTY_BYTE, sizeof(TransferState));
    uint16_t fileSize = ReadDataFileSize(SelectedApp.Slot, FileIndex);
    /* Setup data source */
    TransferState.ReadData.Source.Func = &ReadDataEEPROMSource;
    if (Length == 0) {
        TransferState.ReadData.Encryption.FirstPaddingBitSet = true;
        TransferState.ReadData.BytesLeft = fileSize - Offset;
    } else {
        TransferState.ReadData.Encryption.FirstPaddingBitSet = false;
        TransferState.ReadData.BytesLeft = Length;
    }
    /* Clean data is always located in the beginning of data area */
    TransferState.ReadData.Source.Pointer = GetFileDataAreaBlockId(FileIndex);
    /* Setup data filter */
    return ReadDataFilterSetup(CommSettings);
}

uint8_t WriteDataFileSetup(uint8_t FileIndex, uint8_t FileType, uint8_t CommSettings, uint16_t Offset, uint16_t Length) {
    memset(&TransferState, PICC_EMPTY_BYTE, sizeof(TransferState));
    /* Verify boundary conditions */
    uint16_t fileSize = ReadDataFileSize(SelectedApp.Slot, FileIndex);
    if (Offset + Length > fileSize) {
        return STATUS_BOUNDARY_ERROR;
    }
    /* Setup data sink */
    TransferState.WriteData.BytesLeft = Length;
    TransferState.WriteData.Sink.Func = &WriteDataEEPROMSink;
    TransferState.WriteData.Sink.Pointer = GetFileDataAreaBlockId(FileIndex);
    /* Setup data filter */
    return WriteDataFilterSetup(CommSettings);
}

uint16_t ReadDataFileIterator(uint8_t *Buffer) {
    /* NOTE: incoming ByteCount is not verified here for now */
    TransferStatus Status;
    Status = ReadDataFileTransfer(&Buffer[1]);
    if (Status.IsComplete) {
        Buffer[0] = STATUS_OPERATION_OK;
        DesfireState = DESFIRE_IDLE;
    } else {
        Buffer[0] = STATUS_ADDITIONAL_FRAME;
        DesfireState = DESFIRE_READ_DATA_FILE;
    }
    return DESFIRE_STATUS_RESPONSE_SIZE + Status.BytesProcessed;
}

uint8_t WriteDataFileInternal(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    Status = WriteDataFileTransfer(Buffer, ByteCount);
    switch (Status) {
        case STATUS_OPERATION_OK:
            DesfireState = DESFIRE_IDLE;
            break;
        case STATUS_ADDITIONAL_FRAME:
            DesfireState = DESFIRE_WRITE_DATA_FILE;
            break;
        default:
            break;
    }
    return Status;
}

uint16_t WriteDataFileIterator(uint8_t *Buffer, uint16_t ByteCount) {
    return WriteDataFileInternal(Buffer, ByteCount);
}

uint8_t CreateFileCommonValidation(uint8_t FileNum) {
    /* Validate file number */
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex < DESFIRE_MAX_FILES) {
        return STATUS_DUPLICATE_ERROR;
    }
    /* Validate basic access permissions */
    if (!Authenticated || (IsPiccAppSelected() && AuthenticatedWithKey != 0x00)) {
        return STATUS_AUTHENTICATION_ERROR;
    }
    uint8_t selectedKeyPerms = ReadKeySettings(SelectedApp.Slot, AuthenticatedWithKey);
    if ((selectedKeyPerms & DESFIRE_FREE_CREATE_DELETE) == 0x00) {
        return STATUS_PERMISSION_DENIED;
    }
    return STATUS_OPERATION_OK;
}

uint8_t ValidateAuthentication(uint16_t AccessRights, uint16_t CheckMask) {
    uint8_t SplitPerms[] = {
        GetReadPermissions(CheckMask & AccessRights),
        GetWritePermissions(CheckMask & AccessRights),
        GetReadWritePermissions(CheckMask & AccessRights),
        GetChangePermissions(CheckMask & AccessRights)
    };
    bool PermsRelevant[] = {
        CheckMask & VALIDATE_ACCESS_READ,
        CheckMask & VALIDATE_ACCESS_WRITE,
        CheckMask & VALIDATE_ACCESS_READWRITE,
        CheckMask &VALIDATE_ACCESS_CHANGE
    };
    for (int bpos = 0; bpos < 4; bpos++) {
        if (!PermsRelevant[bpos]) {
            continue;
        } else if (SplitPerms[bpos] == AuthenticatedWithKey) {
            return VALIDATED_ACCESS_GRANTED;
        } else if (SplitPerms[bpos] == DESFIRE_ACCESS_FREE) {
            return VALIDATED_ACCESS_GRANTED_PLAINTEXT;
        }
    }
    return VALIDATED_ACCESS_DENIED;
}

const char *GetFileAccessPermissionsDesc(uint16_t fileAccessRights) {
    __InternalStringBuffer[0] = '\0';
    BYTE removeTrailingText = 0x00;
    if (GetReadPermissions(fileAccessRights) != DESFIRE_ACCESS_DENY) {
        strcat_P(__InternalStringBuffer, PSTR("R/"));
        removeTrailingText = 0x01;
    }
    if (GetWritePermissions(fileAccessRights) != DESFIRE_ACCESS_DENY) {
        strcat_P(__InternalStringBuffer, PSTR("W/"));
        removeTrailingText = 0x01;

    }
    if (GetReadWritePermissions(fileAccessRights) != DESFIRE_ACCESS_DENY) {
        strcat_P(__InternalStringBuffer, PSTR("RW/"));
        removeTrailingText = 0x01;
    }
    if (GetChangePermissions(fileAccessRights) != DESFIRE_ACCESS_DENY) {
        strcat_P(__InternalStringBuffer, PSTR("CHG"));
        removeTrailingText = 0x00;
    }
    if (removeTrailingText) {
        BYTE bufSize = StringLength(__InternalStringBuffer, STRING_BUFFER_SIZE);
        __InternalStringBuffer[bufSize - 1] = '\0';
    }
    return __InternalStringBuffer;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
