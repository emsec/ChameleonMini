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
 * DESFirePICCControl.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../../Settings.h"
#include "../../Log.h"
#include "../../Random.h"
#include "../CryptoTDEA.h"

#include "DESFirePICCControl.h"
#include "DESFirePICCHeaderLayout.h"
#include "DESFireApplicationDirectory.h"
#include "DESFireStatusCodes.h"
#include "DESFireISO14443Support.h"
#include "DESFireMemoryOperations.h"
#include "DESFireUtils.h"
#include "DESFireCrypto.h"
#include "DESFireCryptoTests.h"
#include "DESFireLogging.h"

BYTE SELECTED_APP_CACHE_TYPE_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(sizeof(SelectedAppCacheType));
BYTE APP_CACHE_KEY_SETTINGS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_KEYS);
BYTE APP_CACHE_FILE_NUMBERS_HASHMAP_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_FILES);
BYTE APP_CACHE_FILE_COMM_SETTINGS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_FILES);
BYTE APP_CACHE_FILE_ACCESS_RIGHTS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_FILES);
BYTE APP_CACHE_KEY_VERSIONS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_KEYS);
BYTE APP_CACHE_KEY_TYPES_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(DESFIRE_MAX_KEYS);
BYTE APP_CACHE_KEY_BLOCKIDS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(2 * DESFIRE_MAX_KEYS);
BYTE APP_CACHE_FILE_BLOCKIDS_ARRAY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(2 * DESFIRE_MAX_KEYS);
BYTE APP_CACHE_MAX_KEY_BLOCK_SIZE = DESFIRE_BYTES_TO_BLOCKS(CRYPTO_MAX_KEY_SIZE);

SIZET DESFIRE_PICC_INFO_BLOCK_ID = 0;
SIZET DESFIRE_APP_DIR_BLOCK_ID = 0;
SIZET DESFIRE_INITIAL_FIRST_FREE_BLOCK_ID = 0;
SIZET DESFIRE_FIRST_FREE_BLOCK_ID = 0;
SIZET CardCapacityBlocks = 0;

bool DesfireATQAReset = false;

void InitBlockSizes(void) {
    DESFIRE_PICC_INFO_BLOCK_ID = 0;
    DESFIRE_APP_DIR_BLOCK_ID = DESFIRE_PICC_INFO_BLOCK_ID +
                               DESFIRE_BYTES_TO_BLOCKS(sizeof(DESFirePICCInfoType));
    DESFIRE_FIRST_FREE_BLOCK_ID = DESFIRE_APP_DIR_BLOCK_ID +
                                  DESFIRE_BYTES_TO_BLOCKS(sizeof(DESFireAppDirType));
    DESFIRE_INITIAL_FIRST_FREE_BLOCK_ID = DESFIRE_FIRST_FREE_BLOCK_ID;
}

DESFirePICCInfoType Picc = { 0 };
DESFireAppDirType AppDir = { 0 };
SelectedAppCacheType SelectedApp = { 0 };
SelectedFileCacheType SelectedFile = { 0 };
TransferStateType TransferState = { 0 };

/* Transfer routines */

void SynchronizePICCInfo(void) {
    WriteBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(DESFirePICCInfoType));
}

TransferStatus PiccToPcdTransfer(uint8_t *Buffer) {
    TransferStatus Status;
    uint8_t XferBytes;
    if (TransferState.ReadData.BytesLeft) {
        if (TransferState.ReadData.BytesLeft > DESFIRE_MAX_PAYLOAD_SIZE) {
            XferBytes = DESFIRE_MAX_PAYLOAD_SIZE;
        } else {
            XferBytes = (uint8_t) TransferState.ReadData.BytesLeft;
        }
        /* Read input bytes */
        TransferState.ReadData.Source.Func(Buffer, XferBytes);
        TransferState.ReadData.BytesLeft -= XferBytes;
        Status.BytesProcessed = XferBytes;
        Status.IsComplete = TransferState.ReadData.BytesLeft == 0;
    } else {
        /* Final encryption block */
        Status.IsComplete = true;
        Status.BytesProcessed = 0;
        Status.IsComplete = true;
    }
    return Status;
}

uint8_t PcdToPiccTransfer(uint8_t *Buffer, uint8_t Count) {
    TransferState.WriteData.Sink.Func(Buffer, Count);
    return STATUS_OPERATION_OK;
}

uint8_t ReadDataFilterSetup(uint8_t CommSettings) {
    switch (CommSettings) {
        case DESFIRE_COMMS_PLAINTEXT:
            break;
        case DESFIRE_COMMS_PLAINTEXT_MAC:
            memset(SessionIV, PICC_EMPTY_BYTE, sizeof(SessionIV));
            SessionIVByteSize = CRYPTO_2KTDEA_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_DES:
            memset(SessionIV, PICC_EMPTY_BYTE, sizeof(SessionIV));
            SessionIVByteSize = CRYPTO_3KTDEA_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_AES128:
            memset(SessionIV, 0, sizeof(SessionIVByteSize));
            SessionIVByteSize = CRYPTO_AES_KEY_SIZE;
        default:
            return STATUS_PARAMETER_ERROR;
    }
    return STATUS_OPERATION_OK;
}

uint8_t WriteDataFilterSetup(uint8_t CommSettings) {
    switch (CommSettings) {
        case DESFIRE_COMMS_PLAINTEXT:
            memset(SessionIV, 0, sizeof(SessionIVByteSize));
            SessionIVByteSize = 0;
            break;
        case DESFIRE_COMMS_PLAINTEXT_MAC:
            memset(SessionIV, 0, sizeof(SessionIVByteSize));
            SessionIVByteSize = CRYPTO_2KTDEA_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_DES:
            memset(SessionIV, 0, sizeof(SessionIVByteSize));
            SessionIVByteSize = CRYPTO_AES_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_AES128:
            memset(SessionIV, 0, sizeof(SessionIVByteSize));
            SessionIVByteSize = CRYPTO_AES_KEY_SIZE;
            break;
        default:
            return STATUS_PARAMETER_ERROR;
    }
    return STATUS_OPERATION_OK;
}

/*
 * PICC management routines
 */

void InitialisePiccBackendEV0(uint8_t StorageSize, bool formatPICC) {
#ifdef DESFIRE_RUN_CRYPTO_TESTING_PROCEDURE
    RunCryptoUnitTests();
#endif
    /* Init backend */
    InitBlockSizes();
    CardCapacityBlocks = StorageSize;
    MemoryRecall();
    ReadBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(DESFirePICCInfoType));
    if (formatPICC) {
        DesfireLogEntry(LOG_INFO_DESFIRE_PICC_RESET, (void *) NULL, 0);
        FactoryFormatPiccEV0();
    } else {
        MemoryRestoreDesfireHeaderBytes(false);
        ReadBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(DESFireAppDirType));
        DesfireATQAReset = true;
        SelectPiccApp();
    }
}

void InitialisePiccBackendEV1(uint8_t StorageSize, bool formatPICC) {
#ifdef DESFIRE_RUN_CRYPTO_TESTING_PROCEDURE
    RunCryptoUnitTests();
#endif
    /* Init backend */
    InitBlockSizes();
    CardCapacityBlocks = StorageSize;
    MemoryRecall();
    ReadBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(DESFirePICCInfoType));
    if (formatPICC) {
        DesfireLogEntry(LOG_INFO_DESFIRE_PICC_RESET, (void *) NULL, 0);
        FactoryFormatPiccEV1(StorageSize);
    } else {
        MemoryRestoreDesfireHeaderBytes(false);
        ReadBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(DESFireAppDirType));
        DesfireATQAReset = true;
        SelectPiccApp();
    }
}

void InitialisePiccBackendEV2(uint8_t StorageSize, bool formatPICC) {
#ifdef DESFIRE_RUN_CRYPTO_TESTING_PROCEDURE
    RunCryptoUnitTests();
#endif
    /* Init backend */
    InitBlockSizes();
    CardCapacityBlocks = StorageSize;
    MemoryRecall();
    ReadBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(DESFirePICCInfoType));
    if (formatPICC) {
        DesfireLogEntry(LOG_INFO_DESFIRE_PICC_RESET, (void *) NULL, 0);
        FactoryFormatPiccEV2(StorageSize);
    } else {
        MemoryRestoreDesfireHeaderBytes(false);
        ReadBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(DESFireAppDirType));
        DesfireATQAReset = true;
        SelectPiccApp();
    }

}

void ResetPiccBackend(void) {
    InitBlockSizes();
    SelectPiccApp();
}

bool IsEmulatingEV1(void) {
    return Picc.HwVersionMajor >= DESFIRE_HW_MAJOR_EV1;
}

void GetPiccHardwareVersionInfo(uint8_t *Buffer) {
    Buffer[0] = Picc.HwVersionMajor;
    Buffer[1] = Picc.HwVersionMinor;
    Buffer[2] = DESFIRE_STORAGE_SIZE_8K;
}

void GetPiccSoftwareVersionInfo(uint8_t *Buffer) {
    Buffer[0] = Picc.SwVersionMajor;
    Buffer[1] = Picc.SwVersionMinor;
    Buffer[2] = Picc.StorageSize;
}

void GetPiccManufactureInfo(uint8_t *Buffer) {
    /* UID / serial number does not depend on card mode: */
    memcpy(Buffer, &Picc.Uid[0], DESFIRE_UID_SIZE);
    memcpy(&Buffer[DESFIRE_UID_SIZE], Picc.BatchNumber, 5);
    Buffer[DESFIRE_UID_SIZE + 5] = Picc.ProductionWeek;
    Buffer[DESFIRE_UID_SIZE + 6] = Picc.ProductionYear;
}

uint8_t GetPiccKeySettings(void) {
    return ReadKeySettings(DESFIRE_PICC_APP_SLOT, 0x00);
}

void FormatPicc(void) {
    /* Wipe application directory */
    memset(&AppDir, 0x00, sizeof(DESFireAppDirType));
    memset(&SelectedApp, 0x00, sizeof(SelectedAppCacheType));
    /* Set a random new UID */
    BYTE uidData[DESFIRE_UID_SIZE];
    RandomGetBuffer(uidData, DESFIRE_UID_SIZE);
    memcpy(&Picc.Uid[0], uidData, DESFIRE_UID_SIZE);
    /* Conform to NXP Application Note AN10927 about the first
     * byte of a randomly generated UID (refer to section 2.1.1).
     */
    Picc.Uid[0] = ISO14443A_UID0_RANDOM;
    uint16_t ATQAValue = DESFIRE_ATQA_RANDOM_UID;
    Picc.ATQA[0] = (uint8_t)((ATQAValue >> 8) & 0x00FF);
    Picc.ATQA[1] = (uint8_t)(ATQAValue & 0x00FF);
    DesfireATQAReset = false;
    /* Randomize the initial batch number data: */
    BYTE batchNumberData[5];
    RandomGetBuffer(batchNumberData, 5);
    memcpy(&Picc.BatchNumber[0], batchNumberData, 5);
    /* Default production date -- until the user changes them: */
    Picc.ProductionWeek = 0x01;
    Picc.ProductionYear = 0x05;
    /* Assign the default manufacturer ID: */
    Picc.ManufacturerID = DESFIRE_MANUFACTURER_ID;
    Picc.HwType = DESFIRE_TYPE;
    Picc.HwSubtype = DESFIRE_SUBTYPE;
    Picc.HwProtocolType = DESFIRE_HW_PROTOCOL_TYPE;
    Picc.SwType = DESFIRE_TYPE;
    Picc.SwSubtype = DESFIRE_SUBTYPE;
    Picc.SwProtocolType = DESFIRE_SW_PROTOCOL_TYPE;
    /* Set the ATS bytes to defaults: */
    Picc.ATSBytes[0] = DESFIRE_EV0_ATS_TL_BYTE;
    Picc.ATSBytes[1] = DESFIRE_EV0_ATS_T0_BYTE;
    Picc.ATSBytes[2] = DESFIRE_EV0_ATS_TA_BYTE;
    Picc.ATSBytes[3] = DESFIRE_EV0_ATS_TB_BYTE;
    Picc.ATSBytes[4] = DESFIRE_EV0_ATS_TC_BYTE;
    /* Set the first free slot to 1 -- slot 0 is the PICC app */
    AppDir.FirstFreeSlot = 0;
    /* Flush the new local struct data out to the FRAM: */
    SynchronizeAppDir();
    /* Initialize the root app data */
    CreatePiccApp();
}

void CreatePiccApp(void) {
    CryptoKeyBufferType Key;
    BYTE MasterAppAID[] = { 0x00, 0x00, 0x00 };
    BYTE statusCode = CreateApp(MasterAppAID, DESFIRE_MAX_KEYS, 0x0f);
    if (statusCode != STATUS_OPERATION_OK) {
        DEBUG_PRINT_P(PSTR("CreateApp returned -- %d\n"), statusCode);
    }
    SelectPiccApp();
    memset(&Key, 0x00, sizeof(CryptoKeyBufferType));
    WriteAppKey(0x00, 0x00, Key, sizeof(CryptoKeyBufferType));
    SynchronizeAppDir();
    SynchronizePICCInfo();
}

void FactoryFormatPiccEV0(void) {
    /* Wipe PICC data */
    memset(&Picc, PICC_FORMAT_BYTE, sizeof(Picc));
    /* Initialize params to look like EV0 */
    Picc.StorageSize = CardCapacityBlocks;
    Picc.HwVersionMajor = DESFIRE_HW_MAJOR_EV0;
    Picc.HwVersionMinor = DESFIRE_HW_MINOR_EV0;
    Picc.SwVersionMajor = DESFIRE_SW_MAJOR_EV0;
    Picc.SwVersionMinor = DESFIRE_SW_MINOR_EV0;
    /* Reset the free block pointer */
    Picc.FirstFreeBlock = DESFIRE_FIRST_FREE_BLOCK_ID;
    /* Continue with user data initialization */
    SynchronizePICCInfo();
    FormatPicc();
}

void FactoryFormatPiccEV1(uint8_t StorageSize) {
    /* Wipe PICC data */
    memset(&Picc, PICC_FORMAT_BYTE, sizeof(Picc));
    /* Initialize params to look like EV1 */
    Picc.StorageSize = StorageSize;
    Picc.HwVersionMajor = DESFIRE_HW_MAJOR_EV1;
    Picc.HwVersionMinor = DESFIRE_HW_MINOR_EV1;
    Picc.SwVersionMajor = DESFIRE_SW_MAJOR_EV1;
    Picc.SwVersionMinor = DESFIRE_SW_MINOR_EV1;
    /* Reset the free block pointer */
    Picc.FirstFreeBlock = DESFIRE_FIRST_FREE_BLOCK_ID;
    /* Continue with user data initialization */
    SynchronizePICCInfo();
    FormatPicc();
}

void FactoryFormatPiccEV2(uint8_t StorageSize) {
    /* Wipe PICC data */
    memset(&Picc, PICC_FORMAT_BYTE, sizeof(Picc));
    /* Initialize params to look like EV1 */
    Picc.StorageSize = StorageSize;
    Picc.HwVersionMajor = DESFIRE_HW_MAJOR_EV2;
    Picc.HwVersionMinor = DESFIRE_HW_MINOR_EV2;
    Picc.SwVersionMajor = DESFIRE_SW_MAJOR_EV2;
    Picc.SwVersionMinor = DESFIRE_SW_MINOR_EV2;
    /* Reset the free block pointer */
    Picc.FirstFreeBlock = DESFIRE_FIRST_FREE_BLOCK_ID;
    /* Continue with user data initialization */
    FormatPicc();
    SynchronizePICCInfo();
}

void GetPiccUid(ConfigurationUidType Uid) {
    memcpy(Uid, &Picc.Uid[0], DESFIRE_UID_SIZE);
}

void SetPiccUid(ConfigurationUidType Uid) {
    memcpy(&Picc.Uid[0], Uid, DESFIRE_UID_SIZE);
    if (!DesfireATQAReset && Uid[0] != ISO14443A_UID0_RANDOM) {
        uint16_t ATQAValue = DESFIRE_ATQA_DEFAULT;
        Picc.ATQA[0] = (uint8_t)((ATQAValue >> 8) & 0x00FF);
        Picc.ATQA[1] = (uint8_t)(ATQAValue & 0x00FF);
        DesfireATQAReset = true;
    } else if (!DesfireATQAReset && Uid[0] == ISO14443A_UID0_RANDOM) {
        uint16_t ATQAValue = DESFIRE_ATQA_RANDOM_UID;
        Picc.ATQA[0] = (uint8_t)((ATQAValue >> 8) & 0x00FF);
        Picc.ATQA[1] = (uint8_t)(ATQAValue & 0x00FF);
    }
    SynchronizePICCInfo();
    MemoryStoreDesfireHeaderBytes();
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
