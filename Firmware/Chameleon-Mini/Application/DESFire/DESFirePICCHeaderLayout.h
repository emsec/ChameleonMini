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
 * DESFirePICCHeaderLayout.h
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_PICC_HDRLAYOUT_H__
#define __DESFIRE_PICC_HDRLAYOUT_H__

#include "DESFireFirmwareSettings.h"
#include "DESFireISO14443Support.h"

#define DESFIRE_PICC_APP_SLOT           0x00
#define DESFIRE_MASTER_KEY_ID           0x00

#define DESFIRE_NATIVE_CLA              0x90
#define DESFIRE_ISO7816_CLA             0x00

/* Storage allocation constants */
#define DESFIRE_BLOCK_SIZE              (1)  /* Bytes */
#define DESFIRE_BYTES_TO_BLOCKS(x)      (((x) + DESFIRE_BLOCK_SIZE - 1) / DESFIRE_BLOCK_SIZE)

#define DESFIRE_UID_SIZE                ISO14443A_UID_SIZE_DOUBLE
#define DESFIRE_MAX_PAYLOAD_SIZE        (64) /* Bytes */

/*
 * Definitions pertaining to on-card data
 */

/* Anticollision parameters */
#define DESFIRE_ATQA_DEFAULT            0x0344
#define DESFIRE_ATQA_RANDOM_UID         0x0304
extern bool DesfireATQAReset;

#define SAK_CL1_VALUE                   (ISO14443A_SAK_INCOMPLETE_NOT_COMPLIANT)
#define SAK_CL2_VALUE                   (ISO14443A_SAK_COMPLETE_NOT_COMPLIANT)

#define STATUS_FRAME_SIZE               (1 * 8) /* Bits */

#define DESFIRE_EV0_ATS_TL_BYTE         0x06 /* TL: ATS length, 6 bytes */
#define DESFIRE_EV0_ATS_T0_BYTE         0x75 /* T0: TA, TB, TC present; max accepted frame is 64 bytes */
#define DESFIRE_EV0_ATS_TA_BYTE         0x00 /* TA: Only the lowest bit rate is supported (normal is 0x77) */
#define DESFIRE_EV0_ATS_TB_BYTE         0x81 /* TB: taken from the DESFire spec */
#define DESFIRE_EV0_ATS_TC_BYTE         0x02 /* TC: taken from the DESFire spec */

/* Defines for GetVersion */
#define ID_PHILIPS_NXP                  0x04
#define DESFIRE_MANUFACTURER_ID         ID_PHILIPS_NXP

/* These do not change */
#define DESFIRE_TYPE                    0x01
#define DESFIRE_SUBTYPE                 0x01
#define DESFIRE_HW_PROTOCOL_TYPE        0x05
#define DESFIRE_SW_PROTOCOL_TYPE        0x05

/** Source: http://www.proxmark.org/forum/viewtopic.php?id=2982 **/
#define MIFARE_DESFIRE_EV0              0x00
#define MIFARE_DESFIRE_EV1              0x01
#define MIFARE_DESFIRE_EV2              0x02

/* DESFire EV0 versions */
#define DESFIRE_HW_MAJOR_EV0            0x00
#define DESFIRE_HW_MINOR_EV0            0x01
#define DESFIRE_SW_MAJOR_EV0            0x00
#define DESFIRE_SW_MINOR_EV0            0x01

#define IsPiccEV0(picc)                              \
     (picc.HwVersionMajor == DESFIRE_HW_MAJOR_EV0 && \
      picc.SwVersionMajor == DESFIRE_SW_MAJOR_EV0)

/* DESFire EV1 versions */
#define DESFIRE_HW_MAJOR_EV1            0x01
#define DESFIRE_HW_MINOR_EV1            0x01
#define DESFIRE_SW_MAJOR_EV1            0x01
#define DESFIRE_SW_MINOR_EV1            0x01

/* DESFire EV2 versions */
#define DESFIRE_HW_MAJOR_EV2            0x12
#define DESFIRE_HW_MINOR_EV2            0x01
#define DESFIRE_SW_MAJOR_EV2            0x12
#define DESFIRE_SW_MINOR_EV2            0x01

#define DESFIRE_STORAGE_SIZE_2K         0x16
#define DESFIRE_STORAGE_SIZE_4K         0x18
#define DESFIRE_STORAGE_SIZE_8K         0x1A

/*
 * Defines the global PICC configuration.
 * This is located in the very first block on the card.
 */
#define PICC_FORMAT_BYTE                0x00
#define PICC_EMPTY_BYTE                 0x00

typedef struct DESFIRE_FIRMWARE_PACKING DESFIRE_FIRMWARE_ALIGNAT {
    /* Static data: does not change during the PICC's lifetime.
     * We will add Chameleon Mini terminal commands to enable
     * resetting this data so tags can be emulated authentically.
     * This structure is stored verbatim (using memcpy) at the
     * start of the FRAM setting space for the configuration.
     */
    uint8_t Uid[DESFIRE_UID_SIZE] DESFIRE_FIRMWARE_ALIGNAT;
    uint8_t StorageSize;
    uint8_t ManufacturerID;
    uint8_t HwType;
    uint8_t HwSubtype;
    uint8_t HwVersionMajor;
    uint8_t HwVersionMinor;
    uint8_t HwProtocolType;
    uint8_t SwType;
    uint8_t SwSubtype;
    uint8_t SwVersionMajor;
    uint8_t SwVersionMinor;
    uint8_t SwProtocolType;
    uint8_t BatchNumber[5] DESFIRE_FIRMWARE_ALIGNAT;
    uint8_t ProductionWeek;
    uint8_t ProductionYear;
    uint8_t ATQA[2];
    uint8_t ATSBytes[5];
    /* Dynamic data: changes during the PICC's lifetime */
    uint16_t FirstFreeBlock;
    uint8_t TransactionStarted;
} DESFirePICCInfoType;

typedef struct DESFIRE_FIRMWARE_PACKING DESFIRE_FIRMWARE_ALIGNAT {
    BYTE  Slot;
    BYTE  KeyCount;
    BYTE  MaxKeyCount;
    BYTE  FileCount;
    BYTE  CryptoCommStandard;
    SIZET KeySettings;            /* Block offset in FRAM */
    SIZET FileNumbersArrayMap;    /* Block offset in FRAM */
    SIZET FileCommSettings;       /* Block offset in FRAM */
    SIZET FileAccessRights;       /* Block offset in FRAM */
    SIZET FilesAddress;           /* Block offset in FRAM */
    SIZET KeyVersionsArray;       /* Block offset in FRAM */
    SIZET KeyTypesArray;          /* Block offset in FRAM */
    SIZET KeyAddress;             /* Block offset in FRAM */
} SelectedAppCacheType;

extern BYTE SELECTED_APP_CACHE_TYPE_BLOCK_SIZE;
extern BYTE APP_CACHE_KEY_SETTINGS_ARRAY_BLOCK_SIZE;
extern BYTE APP_CACHE_FILE_NUMBERS_HASHMAP_BLOCK_SIZE;
extern BYTE APP_CACHE_FILE_COMM_SETTINGS_ARRAY_BLOCK_SIZE;
extern BYTE APP_CACHE_FILE_ACCESS_RIGHTS_ARRAY_BLOCK_SIZE;
extern BYTE APP_CACHE_KEY_VERSIONS_ARRAY_BLOCK_SIZE;
extern BYTE APP_CACHE_KEY_TYPES_ARRAY_BLOCK_SIZE;
extern BYTE APP_CACHE_KEY_BLOCKIDS_ARRAY_BLOCK_SIZE;
extern BYTE APP_CACHE_FILE_BLOCKIDS_ARRAY_BLOCK_SIZE;
extern BYTE APP_CACHE_MAX_KEY_BLOCK_SIZE;

extern SIZET DESFIRE_PICC_INFO_BLOCK_ID;
extern SIZET DESFIRE_APP_DIR_BLOCK_ID;
extern SIZET DESFIRE_INITIAL_FIRST_FREE_BLOCK_ID;
extern SIZET DESFIRE_FIRST_FREE_BLOCK_ID;
extern SIZET CardCapacityBlocks;

typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
    /* AppData keeping track how many keys each app has */
    DESFIRE_APP_KEY_COUNT,
    DESFIRE_APP_MAX_KEY_COUNT,
    /* AppData active file count */
    DESFIRE_APP_FILE_COUNT,
    /* AppData keep track of default crypto comm standard */
    DESFIRE_APP_CRYPTO_COMM_STANDARD,
    /* AppData keeping track of apps key settings */
    DESFIRE_APP_KEY_SETTINGS_BLOCK_ID,
    /* AppData hash-like unsorted array mapping file indices to their labeled numbers */
    DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID,
    /* AppData communication settings (crypto transfer protocols) per file */
    DESFIRE_APP_FILE_COMM_SETTINGS_BLOCK_ID,
    /* AppData file access rights */
    DESFIRE_APP_FILE_ACCESS_RIGHTS_BLOCK_ID,
    /* AppData keep track of newer EVx revisions key versioning schemes */
    DESFIRE_APP_KEY_VERSIONS_ARRAY_BLOCK_ID,
    /* AppData keep track of the key types (and hence, byte sizes) by crypto method */
    DESFIRE_APP_KEY_TYPES_ARRAY_BLOCK_ID,
    /* AppData keeping track of apps file index blocks */
    DESFIRE_APP_FILES_PTR_BLOCK_ID,
    /* AppData keeping track of apps key locations */
    DESFIRE_APP_KEYS_PTR_BLOCK_ID,
} DesfireCardLayout;

#endif
