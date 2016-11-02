/*
 * DesfireImpl.h
 * MIFARE DESFire backend
 *
 *  Created on: 02.11.2016
 *      Author: dev_zzo
 */

#include "ISO14443-3A.h"
#include "CryptoTDEA.h"
#include "../Configuration.h"
#include "../Memory.h"
#include "../Random.h"

#define ISO7816_4_CURRENT_EF_FILE_ID    0x0000
#define ISO7816_4_CURRENT_DF_FILE_ID    0x3FFF
#define ISO7816_4_MASTER_FILE_ID        0x3F00

/* Various limits */

#define DESFIRE_UID_SIZE     ISO14443A_UID_SIZE_DOUBLE

#define DESFIRE_AID_SIZE 3

#define DESFIRE_MAX_APPS        28
#define DESFIRE_MAX_KEYS        14
#define DESFIRE_MAX_FILES_EV0   16
#define DESFIRE_MAX_FILES_EV1   32
#define DESFIRE_MAX_FILES       DESFIRE_MAX_FILES_EV1

/* File types */

#define DESFIRE_FILE_STANDARD_DATA      0
#define DESFIRE_FILE_BACKUP_DATA        1
#define DESFIRE_FILE_VALUE_DATA         2
#define DESFIRE_FILE_LINEAR_RECORDS     3
#define DESFIRE_FILE_CIRCULAR_RECORDS   4

/* Special values for access key IDs */

#define DESFIRE_ACCESS_FREE     0xE
#define DESFIRE_ACCESS_DENY     0xF

#define DESFIRE_CHANGE_ACCESS_RIGHTS_SHIFT      (0*4)
#define DESFIRE_READWRITE_ACCESS_RIGHTS_SHIFT   (1*4)
#define DESFIRE_WRITE_ACCESS_RIGHTS_SHIFT       (2*4)
#define DESFIRE_READ_ACCESS_RIGHTS_SHIFT        (3*4)

#define DESFIRE_COMMS_PLAINTEXT         0
#define DESFIRE_COMMS_PLAINTEXT_MAC     1
#define DESFIRE_COMMS_CIPHERTEXT_DES    3

/* PICC / Application master key settings */
#define DESFIRE_ALLOW_MASTER_KEY_CHANGE  (1 << 0)
#define DESFIRE_FREE_DIRECTORY_LIST      (1 << 1)
#define DESFIRE_FREE_CREATE_DELETE       (1 << 2)
#define DESFIRE_ALLOW_CONFIG_CHANGE      (1 << 3)
#define DESFIRE_USE_TARGET_KEY           0xE
#define DESFIRE_ALL_KEYS_FROZEN          0xF

#define DESFIRE_XFER_LAST_BLOCK 0x8000

/* Storage allocation constants */

#define DESFIRE_EEPROM_BLOCK_SIZE 32 /* Bytes */
#define DESFIRE_BYTES_TO_BLOCKS(x) \
    ( ((x) + DESFIRE_EEPROM_BLOCK_SIZE - 1) / DESFIRE_EEPROM_BLOCK_SIZE )

/* Source: http://www.proxmark.org/forum/viewtopic.php?id=2982 */
/* DESFire EV0 versions */
#define DESFIRE_HW_MAJOR_EV0     0x00
#define DESFIRE_HW_MINOR_EV0     0x01
#define DESFIRE_SW_MAJOR_EV0     0x00
#define DESFIRE_SW_MINOR_EV0     0x01
/* DESFire EV1 versions */
#define DESFIRE_HW_MAJOR_EV1     0x01
#define DESFIRE_HW_MINOR_EV1     0x01
#define DESFIRE_SW_MAJOR_EV1     0x01
#define DESFIRE_SW_MINOR_EV1     0x01
/* DESFire EV2 versions */
#define DESFIRE_HW_MAJOR_EV2     0x12
#define DESFIRE_HW_MINOR_EV2     0x01
#define DESFIRE_SW_MAJOR_EV2     0x12
#define DESFIRE_SW_MINOR_EV2     0x01

#define DESFIRE_STORAGE_SIZE_2K  0x16
#define DESFIRE_STORAGE_SIZE_4K  0x18
#define DESFIRE_STORAGE_SIZE_8K  0x1A

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

/*
 * Definitions pertaining to on-card data
 */

typedef uint8_t DesfireAidType[DESFIRE_AID_SIZE];


typedef uint8_t Desfire2KTDEAKeyType[CRYPTO_2KTDEA_KEY_SIZE];
typedef uint8_t Desfire3KTDEAKeyType[CRYPTO_3KTDEA_KEY_SIZE];


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
        } BackupFile;
        struct {
            int32_t LowerLimit;
            int32_t UpperLimit;
            int32_t CleanValue;
            int32_t DirtyValue;
            uint8_t LimitedCreditEnabled;
            int32_t PreviousDebit;
        } ValueFile;
        struct {
            uint8_t BlockCount;
            uint8_t ClearPending;
            uint16_t RecordSize;
            uint16_t CurrentRecord;
            uint16_t MaxRecordCount;
        } RecordFile;
    };
} DesfireFileType;

typedef uint8_t DesfireFileIndexType[DESFIRE_MAX_FILES];


#define DESFIRE_PICC_APP_SLOT 0
#define DESFIRE_MAX_SLOTS (DESFIRE_MAX_APPS + 1)

/** Data about applications is kept in these structures */
typedef struct {
    uint8_t Spare0;
    uint8_t AppData[DESFIRE_MAX_SLOTS];
    uint16_t Checksum; /* Not actually used atm */
} DesfireApplicationDataType;


/** Defines the global PICC configuration. */
typedef struct {
    /* Static data: does not change during the PICC's lifetime */
    uint8_t Uid[DESFIRE_UID_SIZE];
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
    uint8_t TransactionStarted;
    uint8_t Spare[9];
} DesfirePiccInfoType;

/** Defines the application directory contents. */
typedef struct {
    uint8_t FirstFreeSlot;
    uint8_t Spare[8];
    DesfireAidType AppIds[DESFIRE_MAX_SLOTS]; /* 84 */
} DesfireAppDirType;


/* This resolves to 4 */
#define DESFIRE_APP_DIR_BLOCKS DESFIRE_BYTES_TO_BLOCKS(sizeof(DesfireAppDirType))
/* This resolves to 1 */
#define DESFIRE_FILE_INDEX_BLOCKS DESFIRE_BYTES_TO_BLOCKS(sizeof(DesfireFileIndexType))

/* The actual layout */
enum DesfireCardLayout {
    /* PICC related informaton is kept here */
    DESFIRE_PICC_INFO_BLOCK_ID = 0,
    /* Keeps the list of all applications created */
    DESFIRE_APP_DIR_BLOCK_ID,
    /* AppData keeping track of apps' key settings */
    DESFIRE_APP_KEY_SETTINGS_BLOCK_ID = DESFIRE_APP_DIR_BLOCK_ID + DESFIRE_APP_DIR_BLOCKS,
    /* AppData keeping track how many keys each app has */
    DESFIRE_APP_KEY_COUNT_BLOCK_ID,
    /* AppData keeping track of apps' key locations */
    DESFIRE_APP_KEYS_PTR_BLOCK_ID,
    /* AppData keeping track of apps' file index blocks */
    DESFIRE_APP_FILES_PTR_BLOCK_ID,
    /* Free space starts here */
    DESFIRE_FIRST_FREE_BLOCK_ID,
};

/*
 * DESFire backend API functions
 */

/* Application management */
bool IsPiccAppSelected(void);
void SelectPiccApp(void);
uint8_t SelectApp(const DesfireAidType Aid);
uint8_t CreateApp(const DesfireAidType Aid, uint8_t KeyCount, uint8_t KeySettings);
uint8_t DeleteApp(const DesfireAidType Aid);
void GetApplicationIdsSetup(void);
uint16_t GetApplicationIdsTransfer(uint8_t* Buffer);

/* Application key management */
uint8_t GetSelectedAppKeyCount(void);
uint8_t GetSelectedAppKeySettings(void);
void SetSelectedAppKeySettings(uint8_t KeySettings);
void ReadSelectedAppKey(uint8_t KeyId, uint8_t* Key);
void WriteSelectedAppKey(uint8_t KeyId, const uint8_t* Key);

/* File management */
uint8_t CreateStandardFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize);
uint8_t CreateBackupFile(uint8_t FileNum, uint8_t CommSettings, uint16_t AccessRights, uint16_t FileSize);
uint8_t DeleteFile(uint8_t FileNum);
void CommitTransaction(void);
void AbortTransaction(void);
uint8_t SelectFile(uint8_t FileNum);
uint8_t GetSelectedFileType(void);
uint8_t GetSelectedFileCommSettings(void);
uint16_t GetSelectedFileAccessRights(void);

/* PICC management */
void InitialisePiccBackendEV0(uint8_t StorageSize);
void InitialisePiccBackendEV1(uint8_t StorageSize);
void ResetPiccBackend(void);
bool IsEmulatingEV1(void);
void GetPiccHardwareVersionInfo(uint8_t* Buffer);
void GetPiccSoftwareVersionInfo(uint8_t* Buffer);
void GetPiccManufactureInfo(uint8_t* Buffer);
uint8_t GetPiccKeySettings(void);
void GetPiccUid(ConfigurationUidType Uid);
void SetPiccUid(ConfigurationUidType Uid);
void FormatPicc(void);
void FactoryFormatPiccEV0(void);
void FactoryFormatPiccEV1(uint8_t StorageSize);


/*
 * State shared between frontend and backend
 */

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

extern DesfireStateType DesfireState;
/* TODO: Update with to accomodate other auth modes */
extern Desfire2KTDEAKeyType SessionKey;
extern uint8_t SessionIV[CRYPTO_DES_BLOCK_SIZE];

