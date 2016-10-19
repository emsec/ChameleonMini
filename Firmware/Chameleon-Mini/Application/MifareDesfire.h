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
#define MIFARE_DESFIRE_MAX_APPS_INTERNAL 32
#define MIFARE_DESFIRE_MAX_FILES 16
#define MIFARE_DESFIRE_MAX_KEYS 14

#define MIFARE_DESFIRE_STANDARD_DATA_FILE      0
#define MIFARE_DESFIRE_BACKUP_DATA_FILE        1
#define MIFARE_DESFIRE_VALUE_DATA_FILE         2
#define MIFARE_DESFIRE_LINEAR_RECORDS_FILE     3
#define MIFARE_DESFIRE_CIRCULAR_RECORDS_FILE   4

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
    uint8_t BlockIndex;
    uint16_t AccessRights;
    uint16_t FileSize;
} MifareDesfireFileType;

typedef struct {
    uint8_t Key1[CRYPTO_DES_KEY_SIZE];
    uint8_t Key2[CRYPTO_DES_KEY_SIZE];
} MifareDesfireKeyType;

typedef struct {
    uint8_t KeyCount;
    MifareDesfireKeyType Keys[MIFARE_DESFIRE_MAX_KEYS];
    MifareDesfireFileType Files[MIFARE_DESFIRE_MAX_FILES];
} MifareDesfireApplicationType;

typedef struct {
    uint8_t Uid[MIFARE_DESFIRE_UID_SIZE];
    uint8_t FirstFreeBlock;
    MifareDesfireKeyType MasterKey;
} MifareDesfirePiccType;

/* 3 blocks */
typedef MifareDesfireAidType MifareDesfireAppDirType[MIFARE_DESFIRE_MAX_APPS_INTERNAL];

/* Defines the card image format */
typedef struct {
    MifareDesfirePiccType Picc;
    MifareDesfireAppDirType AppDir;
    uint8_t MasterKeySettings[MIFARE_DESFIRE_MAX_APPS_INTERNAL];
    MifareDesfireApplicationType Applications[MIFARE_DESFIRE_MAX_APPS];
} MifareDesfireImageType;

void MifareDesfireAppInit(void);
void MifareDesfireAppReset(void);
void MifareDesfireAppTask(void);

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareDesfireGetUid(ConfigurationUidType Uid);
void MifareDesfireSetUid(ConfigurationUidType Uid);

#endif /* MIFAREDESFIRE_H_ */
