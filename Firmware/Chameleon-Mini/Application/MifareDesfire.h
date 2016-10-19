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
#include "CryptoDES.h"

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

#define MIFARE_DESFIRE_APP_SLOT_EMPTY 0
#define MIFARE_DESFIRE_APP_SLOT_VALID 1
#define MIFARE_DESFIRE_APP_SLOT_INACTIVE 2

#define MIFARE_DESFIRE_EEPROM_BLOCK_SIZE 32 /* Bytes */

typedef struct {
    uint8_t Type;
    uint8_t Flags;
    uint16_t AccessRights;
    uint16_t FileSize;
    uint8_t DataPointer[2];
} MifareDesfireFileType;
/* This resolves to 4 */
#define MIFARE_DESFIRE_FILE_STATE_BLOCKS (MIFARE_DESFIRE_MAX_FILES * sizeof(MifareDesfireFileType) / MIFARE_DESFIRE_EEPROM_BLOCK_SIZE)

typedef struct {
    uint8_t Key1[CRYPTO_DES_KEY_SIZE];
    uint8_t Key2[CRYPTO_DES_KEY_SIZE];
} MifareDesfireKeyType;

/** Defines application data structure. */
typedef struct {
    uint8_t MasterKeySettings;
    uint8_t KeyCount;
    uint8_t Spare[30];
    /* Next: File control blocks */
    /* Next: Key blocks */
} MifareDesfireApplicationType;

/** Defines the global PICC configuration. */
typedef struct {
    uint8_t Uid[MIFARE_DESFIRE_UID_SIZE];
    uint8_t FirstFreeBlock;
    uint8_t MasterKeySettings;
    uint8_t Spare[6];
    MifareDesfireKeyType MasterKey;
} MifareDesfirePiccInfoType;

/** Defines an entry in the application directory. */
typedef struct {
    MifareDesfireAidType Aid;
    uint8_t Pointer;
} MifareDesfireAppDirEntryType;

/** Defines the application directory contents. */
typedef struct {
    MifareDesfireAppDirEntryType Apps[MIFARE_DESFIRE_MAX_APPS];
    uint8_t Spare[16];
} MifareDesfireAppDirType;

/* The actual layout */
#define MIFARE_DESFIRE_PICC_INFO_BLOCK_ID 0
#define MIFARE_DESFIRE_APP_DIR_BLOCK_ID 1

void MifareDesfireAppInit(void);
void MifareDesfireAppReset(void);
void MifareDesfireAppTask(void);

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareDesfireGetUid(ConfigurationUidType Uid);
void MifareDesfireSetUid(ConfigurationUidType Uid);

#endif /* MIFAREDESFIRE_H_ */
