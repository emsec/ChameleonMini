/*
 * MifareDesfire.h
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#ifndef MIFAREDESFIRE_H_
#define MIFAREDESFIRE_H_

#include "Application.h"
#include "ISO14443-3A.h"
#include "CryptoTDEA.h"

#define MIFARE_DESFIRE_UID_SIZE     ISO14443A_UID_SIZE_DOUBLE

#define MIFARE_DESFIRE_AID_SIZE 3
typedef uint8_t MifareDesfireAidType[MIFARE_DESFIRE_AID_SIZE];

#define MIFARE_DESFIRE_MAX_APPS 28
#define MIFARE_DESFIRE_MAX_FILES 16
#define MIFARE_DESFIRE_MAX_KEYS 14

#define MIFARE_DESFIRE_FILE_STANDARD_DATA      0
#define MIFARE_DESFIRE_FILE_BACKUP_DATA        1
#define MIFARE_DESFIRE_FILE_VALUE_DATA         2
#define MIFARE_DESFIRE_FILE_LINEAR_RECORDS     3
#define MIFARE_DESFIRE_FILE_CIRCULAR_RECORDS   4
#define MIFARE_DESFIRE_FILE_COPY_MASK   (1 << 0)
#define MIFARE_DESFIRE_FILE_DIRTY_MASK  (1 << 1)
#define MIFARE_DESFIRE_FILE_ALLOCATED   (1 << 7)

/* Special values for access key IDs */
#define MIFARE_DESFIRE_ACCESS_FREE 0xE
#define MIFARE_DESFIRE_ACCESS_DENY 0xF

#define MIFARE_DESFIRE_CHANGE_ACCESS_RIGHTS_SHIFT      (0*4)
#define MIFARE_DESFIRE_READWRITE_ACCESS_RIGHTS_SHIFT   (1*4)
#define MIFARE_DESFIRE_WRITE_ACCESS_RIGHTS_SHIFT       (2*4)
#define MIFARE_DESFIRE_READ_ACCESS_RIGHTS_SHIFT        (3*4)

#define MIFARE_DESFIRE_COMMS_PLAINTEXT         0
#define MIFARE_DESFIRE_COMMS_PLAINTEXT_MAC     1
#define MIFARE_DESFIRE_COMMS_CIPHERTEXT_DES    3

/* PICC / Application master key settings */
#define MIFARE_DESFIRE_ALLOW_MASTER_KEY_CHANGE  (1 << 0)
#define MIFARE_DESFIRE_FREE_DIRECTORY_LIST      (1 << 1)
#define MIFARE_DESFIRE_FREE_CREATE_DELETE       (1 << 2)
#define MIFARE_DESFIRE_ALLOW_CONFIG_CHANGE      (1 << 3)
#define MIFARE_DESFIRE_USE_TARGET_KEY           0xE
#define MIFARE_DESFIRE_ALL_KEYS_FROZEN          0xF

#define MIFARE_DESFIRE_APP_SLOT_EMPTY 0
#define MIFARE_DESFIRE_APP_SLOT_VALID 1
#define MIFARE_DESFIRE_APP_SLOT_INACTIVE 2

#define MIFARE_DESFIRE_EEPROM_BLOCK_SIZE 32 /* Bytes */
#define MIFARE_DESFIRE_BYTES_TO_BLOCKS(x) \
    ( ((x) + MIFARE_DESFIRE_EEPROM_BLOCK_SIZE - 1) / MIFARE_DESFIRE_EEPROM_BLOCK_SIZE )

/* Source: http://www.proxmark.org/forum/viewtopic.php?id=2982 */
/* DESFire EV0 versions */
#define MIFARE_DESFIRE_HW_MAJOR_EV0     0x00
#define MIFARE_DESFIRE_HW_MINOR_EV0     0x01
#define MIFARE_DESFIRE_SW_MAJOR_EV0     0x00
#define MIFARE_DESFIRE_SW_MINOR_EV0     0x01
/* DESFire EV1 versions */
#define MIFARE_DESFIRE_HW_MAJOR_EV1     0x01
#define MIFARE_DESFIRE_HW_MINOR_EV1     0x01
#define MIFARE_DESFIRE_SW_MAJOR_EV1     0x01
#define MIFARE_DESFIRE_SW_MINOR_EV1     0x01
/* DESFire EV2 versions */
#define MIFARE_DESFIRE_HW_MAJOR_EV2     0x12
#define MIFARE_DESFIRE_HW_MINOR_EV2     0x01
#define MIFARE_DESFIRE_SW_MAJOR_EV2     0x12
#define MIFARE_DESFIRE_SW_MINOR_EV2     0x01

#define MIFARE_DESFIRE_STORAGE_SIZE_2K  0x16
#define MIFARE_DESFIRE_STORAGE_SIZE_4K  0x18
#define MIFARE_DESFIRE_STORAGE_SIZE_8K  0x1A

/* The following mimics the ISO 7816-4 file system structure */

#define ISO7816_4_CURRENT_EF_FILE_ID    0x0000
#define ISO7816_4_CURRENT_DF_FILE_ID    0x3FFF
#define ISO7816_4_MASTER_FILE_ID        0x3F00

typedef struct {
    uint8_t Type;
    uint8_t CommSettings;
    uint16_t AccessRights;
    union {
        struct {
            uint16_t FileSize;
        } StandardFile;
        struct {
            uint16_t FileSize;
            uint8_t BlockCount;
            uint8_t Dirty;
        } BackupFile;
        struct {
            int32_t LowerLimit;
            int32_t UpperLimit;
            int32_t CleanValue;
            int32_t DirtyValue;
            uint8_t LimitedCreditEnabled;
            uint8_t Dirty;
            int32_t PreviousDebit;
        } ValueFile;
        struct {
            uint8_t BlockCount;
            uint8_t Dirty;
            uint8_t ClearPending;
            uint16_t RecordSize;
            uint16_t MaxRecordCount;
        } LinearRecordFile;
    };
} MifareDesfireFileType;

/* TODO: This doesn't work for EV1... */
typedef uint8_t MifareDesfireKeyType[CRYPTO_2KTDEA_KEY_SIZE];

#define MIFARE_DESFIRE_MAX_SLOTS (MIFARE_DESFIRE_MAX_APPS + 1)

/** Data about applications is kept in these structures */
typedef struct {
    uint8_t Spare0;
    uint8_t AppData[MIFARE_DESFIRE_MAX_SLOTS];
    uint16_t Checksum; /* Not actually used atm */
} MifareDesfireApplicationDataType;

/** Defines the global PICC configuration. */
typedef struct {
    /* Static data: does not change during the PICC's lifetime */
    uint8_t Uid[MIFARE_DESFIRE_UID_SIZE];
    uint8_t StorageSize;
    uint8_t HwVersionMajor;
    uint8_t HwVersionMinor;
    uint8_t SwVersionMajor;
    uint8_t SwVersionMinor;
    uint8_t BatchNumber[5];
    uint8_t ProductionWeek;
    uint8_t ProductionYear;
    /* Dynamic data: changes during the PICC's lifetime */
    uint8_t FirstFreeBlock;
    uint8_t Spare[10];
} MifareDesfirePiccInfoType;

/** Defines the application directory contents. */
typedef struct {
    uint8_t FirstFreeSlot;
    uint8_t Spare[8];
    MifareDesfireAidType AppIds[MIFARE_DESFIRE_MAX_SLOTS]; /* 84 */
} MifareDesfireAppDirType;

/* This resolves to 4 */
#define MIFARE_DESFIRE_APP_DIR_BLOCKS MIFARE_DESFIRE_BYTES_TO_BLOCKS(sizeof(MifareDesfireAppDirType))
/* This resolves to 1 */
#define MIFARE_DESFIRE_FILE_INDEX_BLOCKS MIFARE_DESFIRE_BYTES_TO_BLOCKS(MIFARE_DESFIRE_MAX_FILES * sizeof(uint8_t))

/* The actual layout */
enum MifareDesfireCardLayout {
    /* PICC related informaton is kept here */
    MIFARE_DESFIRE_PICC_INFO_BLOCK_ID = 0,
    /* Keeps the list of all applications created */
    MIFARE_DESFIRE_APP_DIR_BLOCK_ID,
    /* AppData keeping track of apps' key settings */
    MIFARE_DESFIRE_APP_KEY_SETTINGS_BLOCK_ID = MIFARE_DESFIRE_APP_DIR_BLOCK_ID + MIFARE_DESFIRE_APP_DIR_BLOCKS,
    /* AppData keeping track how many keys each app has */
    MIFARE_DESFIRE_APP_KEY_COUNT_BLOCK_ID,
    /* AppData keeping track of apps' key locations */
    MIFARE_DESFIRE_APP_KEYS_PTR_BLOCK_ID,
    /* AppData keeping track of apps' file index blocks */
    MIFARE_DESFIRE_APP_FILES_PTR_BLOCK_ID,
    /* Free space starts here */
    MIFARE_DESFIRE_FIRST_FREE_BLOCK_ID,
};

void MifareDesfireAppInit(void);
void MifareDesfireAppReset(void);
void MifareDesfireAppTask(void);

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareDesfireGetUid(ConfigurationUidType Uid);
void MifareDesfireSetUid(ConfigurationUidType Uid);

#endif /* MIFAREDESFIRE_H_ */
