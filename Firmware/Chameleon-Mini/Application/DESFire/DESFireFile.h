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
 * DESFireFile.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_EFFILE_H__
#define __DESFIRE_EFFILE_H__

#include "DESFireFirmwareSettings.h"
#include "DESFireInstructions.h"

#define EFFILE_DATA_CHUNK_SIZE            (4)
#define EFFILE_DATA_CHUNK_ALIGNAT         __attribute__((align(EFFILE_DATA_CHUNK_SIZE)))

// TODO: See type 4 tags on CC and NTAG AIDs and FIDs
#define ISO7816_4_CURRENT_EF_FILE_ID      0x0000
#define ISO7816_4_CURRENT_DF_FILE_ID      0x3FFF
#define ISO7816_4_MASTER_FILE_ID          0x3F00

/* File types */

// NOTE: See pages 48-49 of the datasheet for more details:
#define DESFIRE_FILE_STANDARD_DATA      0
#define DESFIRE_FILE_BACKUP_DATA        1
#define DESFIRE_FILE_VALUE_DATA         2
#define DESFIRE_FILE_LINEAR_RECORDS     3
#define DESFIRE_FILE_CIRCULAR_RECORDS   4

#define DESFIRE_FILE_NOFILE_INDEX       (0xff)

/* Data about an application's file is currently kept in this structure.
 * The location of these structures is defined by the file index.
 */
typedef struct DESFIRE_FIRMWARE_PACKING {
    uint8_t FileType;
    uint8_t FileNumber;
    uint16_t FileSize;
    uint16_t FileDataAddress; /* FRAM address of the storage of the data for the file */
    union DESFIRE_FIRMWARE_ALIGNAT {
        struct DESFIRE_FIRMWARE_ALIGNAT {
            uint16_t FileSize;
        } StandardFile;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            uint16_t FileSize;
            uint8_t BlockCount;
        } BackupFile;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            int32_t CleanValue;
            int32_t DirtyValue;
            int32_t LowerLimit;
            int32_t UpperLimit;
            uint8_t LimitedCreditEnabled;
            int32_t PreviousDebit;
        } ValueFile;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            uint16_t BlockCount;
            uint16_t RecordPointer;
            //uint8_t ClearPending;  // USED ???
            uint8_t RecordSize[3];
            uint8_t CurrentNumRecords[3];
            uint8_t MaxRecordCount[3];
        } RecordFile;
    };
} DESFireFileTypeSettings;

uint16_t GetFileSizeFromFileType(DESFireFileTypeSettings *File);

typedef struct DESFIRE_FIRMWARE_PACKING {
    BYTE Num;
    DESFireFileTypeSettings File;
} SelectedFileCacheType;

/*
 * File management: creation, deletion, and misc routines
 */

uint16_t GetFileDataAreaBlockId(uint8_t FileIndex);
uint8_t ReadFileControlBlockIntoCacheStructure(uint8_t FileNum, SelectedFileCacheType *FileCache);
uint8_t ReadFileControlBlock(uint8_t FileNum, DESFireFileTypeSettings *File);
uint8_t WriteFileControlBlock(uint8_t FileNum, DESFireFileTypeSettings *File);

/* Creation and deletion */
uint8_t CreateFileHeaderData(uint8_t FileNum, uint8_t CommSettings,
                             uint16_t AccessRights, DESFireFileTypeSettings *File);
uint8_t CreateStandardFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize);
uint8_t CreateBackupFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize);
uint8_t CreateValueFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights,
                        int32_t LowerLimit, int32_t UpperLimit, int32_t Value, bool LimitedCreditEnabled);
uint8_t CreateRecordFile(uint8_t FileType, uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights,
                         uint8_t *RecordSize, uint8_t *MaxRecordSize);
uint8_t DeleteFile(uint8_t FileIndex);

/* Transactions */

/* File transfers */
TransferStatus ReadDataFileTransfer(uint8_t *Buffer);
uint8_t WriteDataFileTransfer(uint8_t *Buffer, uint8_t ByteCount);
uint8_t ReadDataFileSetup(uint8_t FileIndex, uint8_t CommSettings, uint16_t Offset, uint16_t Length);
uint8_t WriteDataFileSetup(uint8_t FileIndex, uint8_t FileType, uint8_t CommSettings, uint16_t Offset, uint16_t Length);
uint16_t ReadDataFileIterator(uint8_t *Buffer);
uint8_t WriteDataFileInternal(uint8_t *Buffer, uint16_t ByteCount);
uint16_t WriteDataFileIterator(uint8_t *Buffer, uint16_t ByteCount);

/* Special values for access key IDs */
#define DESFIRE_ACCESS_FREE     0xE
#define DESFIRE_ACCESS_DENY     0xF

/* Validation routines */
#define VALIDATE_ACCESS_READWRITE          ((uint16_t) (0x000f))
#define VALIDATE_ACCESS_WRITE              ((uint16_t) (0x000f << 4))
#define VALIDATE_ACCESS_READ               ((uint16_t) (0x000f << 8))
#define VALIDATE_ACCESS_CHANGE             ((uint16_t) (0x000f << 12))

#define VALIDATED_ACCESS_DENIED            0
#define VALIDATED_ACCESS_GRANTED           1
#define VALIDATED_ACCESS_GRANTED_PLAINTEXT 2

/*
 * [READ (4 bits) | WRITE | READ-WRITE | CHANGE]
 * Stored in little endian format from memory:
 */
#define GetReadPermissions(AccessRights) \
     (BYTE) (AccessRights & 0x000f)
#define GetWritePermissions(AccessRights) \
     (BYTE) (((0x00f0 & AccessRights) >> 4) & 0x000f)
#define GetReadWritePermissions(AccessRights) \
     (BYTE) (((0x0f00 & AccessRights) >> 8) & 0x000f)
#define GetChangePermissions(AccessRights) \
     (BYTE) (((0xf000 & AccessRights) >> 12) & 0x000f)

const char *GetFileAccessPermissionsDesc(uint16_t fileAccessRights);

/*
 * There are also command/instruction-wise
 * citations given from file's access permissions. This data is taken
 * from the table on page 21 of the NXP application note:
 * https://www.nxp.com/docs/en/application-note/AN12343.pdf
 */
uint8_t CreateFileCommonValidation(uint8_t FileNum);
uint8_t ValidateAuthentication(uint16_t AccessRights, uint16_t CheckMask);

#endif
