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
SIZET DESFIRE_APP_CACHE_DATA_ARRAY_BLOCK_ID = 0;
SIZET DESFIRE_INITIAL_FIRST_FREE_BLOCK_ID = 0;
SIZET DESFIRE_FIRST_FREE_BLOCK_ID = 0;
SIZET CardCapacityBlocks = 0;

void InitBlockSizes(void) {
    DESFIRE_PICC_INFO_BLOCK_ID = 0;
    DESFIRE_APP_DIR_BLOCK_ID = DESFIRE_PICC_INFO_BLOCK_ID +
                               DESFIRE_BYTES_TO_BLOCKS(sizeof(DESFirePICCInfoType));
    DESFIRE_APP_CACHE_DATA_ARRAY_BLOCK_ID = DESFIRE_APP_DIR_BLOCK_ID +
                                            DESFIRE_BYTES_TO_BLOCKS(sizeof(DESFireAppDirType));
    DESFIRE_FIRST_FREE_BLOCK_ID = DESFIRE_APP_CACHE_DATA_ARRAY_BLOCK_ID;
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
    //MemoryStore();
}

/* TODO: Currently, everything is transfered in plaintext, without checksums */
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
        /* Update checksum/MAC */
        //if (TransferState.Checksums.UpdateFunc)
        //    TransferState.Checksums.UpdateFunc(Buffer, XferBytes);
        //if (TransferState.ReadData.BytesLeft == 0) {
        //    /* Finalise TransferChecksum and append the checksum */
        //    if (TransferState.Checksums.FinalFunc)
        //        XferBytes += TransferState.Checksums.FinalFunc(&Buffer[XferBytes]);
        //}
        /* Encrypt */
        //Status.BytesProcessed = TransferState.ReadData.Encryption.Func(Buffer, XferBytes);
        //Status.IsComplete = TransferState.ReadData.Encryption.AvailablePlaintext == 0;
        Status.BytesProcessed = XferBytes;
        Status.IsComplete = TransferState.ReadData.BytesLeft == 0;
    } else {
        /* Final encryption block */
        //Status.BytesProcessed = TransferState.ReadData.Encryption.Func(Buffer, 0);
        //Status.IsComplete = true;
        Status.BytesProcessed = 0;
        Status.IsComplete = true;
    }
    return Status;
}

/* TODO: Currently, everything is transfered in plaintext, without checksums */
uint8_t PcdToPiccTransfer(uint8_t *Buffer, uint8_t Count) {
    TransferState.WriteData.Sink.Func(Buffer, Count);
    return STATUS_OPERATION_OK;
}

/* Setup routines */

uint8_t ReadDataFilterSetup(uint8_t CommSettings) {
    switch (CommSettings) {
        case DESFIRE_COMMS_PLAINTEXT:
            break;
        case DESFIRE_COMMS_PLAINTEXT_MAC:
            TransferState.Checksums.UpdateFunc = &TransferChecksumUpdateMACTDEA;
            TransferState.Checksums.FinalFunc = &TransferChecksumFinalMACTDEA;
            TransferState.Checksums.MACData.CryptoChecksumFunc.TDEAFunc = &CryptoEncrypt2KTDEA_CBCSend;
            memset(SessionIV, PICC_EMPTY_BYTE, sizeof(SessionIV));
            SessionIVByteSize = CRYPTO_2KTDEA_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_DES:
            TransferState.Checksums.UpdateFunc = &TransferChecksumUpdateCRCA;
            TransferState.Checksums.FinalFunc = &TransferChecksumFinalCRCA;
            TransferState.Checksums.MACData.CRCA = ISO14443A_CRCA_INIT;
            TransferState.ReadData.Encryption.Func = &TransferEncryptTDEASend;
            memset(SessionIV, PICC_EMPTY_BYTE, sizeof(SessionIV));
            SessionIVByteSize = CRYPTO_3KTDEA_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_AES128:
        default:
            return STATUS_PARAMETER_ERROR;
    }
    return STATUS_OPERATION_OK;
}

uint8_t WriteDataFilterSetup(uint8_t CommSettings) {
    switch (CommSettings) {
        case DESFIRE_COMMS_PLAINTEXT:
            break;
        case DESFIRE_COMMS_PLAINTEXT_MAC:
            TransferState.Checksums.UpdateFunc = &TransferChecksumUpdateMACTDEA;
            TransferState.Checksums.FinalFunc = &TransferChecksumFinalMACTDEA;
            TransferState.Checksums.MACData.CryptoChecksumFunc.TDEAFunc = &CryptoEncrypt2KTDEA_CBCReceive;
            memset(SessionIV, 0, sizeof(SessionIVByteSize));
            SessionIVByteSize = CRYPTO_2KTDEA_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_DES:
            TransferState.Checksums.UpdateFunc = &TransferChecksumUpdateCRCA;
            TransferState.Checksums.FinalFunc = &TransferChecksumFinalCRCA;
            TransferState.Checksums.MACData.CRCA = ISO14443A_CRCA_INIT;
            TransferState.WriteData.Encryption.Func = &TransferEncryptTDEAReceive;
            memset(SessionIV, 0, sizeof(SessionIVByteSize));
            SessionIVByteSize = CRYPTO_3KTDEA_KEY_SIZE;
            break;
        case DESFIRE_COMMS_CIPHERTEXT_AES128:
        default:
            return STATUS_PARAMETER_ERROR;
    }
    return STATUS_OPERATION_OK;
}

/*
 * PICC management routines
 */

void InitialisePiccBackendEV0(uint8_t StorageSize) {
#ifdef DESFIRE_RUN_CRYPTO_TESTING_PROCEDURE
    RunCryptoUnitTests();
#endif
    /* Init backend */
    InitBlockSizes();
    CardCapacityBlocks = StorageSize;
    ReadBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(DESFirePICCInfoType));
    if (Picc.Uid[0] == PICC_FORMAT_BYTE && Picc.Uid[1] == PICC_FORMAT_BYTE &&
            Picc.Uid[2] == PICC_FORMAT_BYTE && Picc.Uid[3] == PICC_FORMAT_BYTE) {
        snprintf_P(__InternalStringBuffer, STRING_BUFFER_SIZE, PSTR("Factory resetting the device"));
        LogEntry(LOG_INFO_DESFIRE_PICC_RESET, (void *) __InternalStringBuffer,
                 StringLength(__InternalStringBuffer, STRING_BUFFER_SIZE));
        FactoryFormatPiccEV0();
    } else {
        ReadBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(DESFireAppDirType));
    }
}

void InitialisePiccBackendEV1(uint8_t StorageSize) {
#ifdef DESFIRE_RUN_CRYPTO_TESTING_PROCEDURE
    RunCryptoUnitTests();
#endif
    /* Init backend */
    InitBlockSizes();
    CardCapacityBlocks = StorageSize;
    ReadBlockBytes(&Picc, DESFIRE_PICC_INFO_BLOCK_ID, sizeof(DESFirePICCInfoType));
    if (Picc.Uid[0] == PICC_FORMAT_BYTE && Picc.Uid[1] == PICC_FORMAT_BYTE &&
            Picc.Uid[2] == PICC_FORMAT_BYTE && Picc.Uid[3] == PICC_FORMAT_BYTE) {
        snprintf_P(__InternalStringBuffer, STRING_BUFFER_SIZE, PSTR("Factory resetting the device"));
        LogEntry(LOG_INFO_DESFIRE_PICC_RESET, (void *) __InternalStringBuffer,
                 StringLength(__InternalStringBuffer, STRING_BUFFER_SIZE));
        FactoryFormatPiccEV1(StorageSize);
    } else {
        ReadBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(DESFireAppDirType));
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
    Buffer[2] = Picc.StorageSize;
}

void GetPiccSoftwareVersionInfo(uint8_t *Buffer) {
    Buffer[0] = Picc.SwVersionMajor;
    Buffer[1] = Picc.SwVersionMinor;
    Buffer[2] = Picc.StorageSize;
}

void GetPiccManufactureInfo(uint8_t *Buffer) {
    /* UID / serial number does not depend on card mode: */
    memcpy(&Buffer[0], &Picc.Uid[0], DESFIRE_UID_SIZE);
    Buffer[7] = Picc.BatchNumber[0];
    Buffer[8] = Picc.BatchNumber[1];
    Buffer[9] = Picc.BatchNumber[2];
    Buffer[10] = Picc.BatchNumber[3];
    Buffer[11] = Picc.BatchNumber[4];
    Buffer[12] = Picc.ProductionWeek;
    Buffer[13] = Picc.ProductionYear;
}

uint8_t GetPiccKeySettings(void) {
    return ReadKeySettings(DESFIRE_PICC_APP_SLOT, 0x00);
}

void FormatPicc(void) {
    /* Wipe application directory */
    memset(&AppDir, 0x00, sizeof(DESFireAppDirType));
    memset(&SelectedApp, 0x00, sizeof(SelectedAppCacheType));
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
        const char *loggingMsg = PSTR("CreateApp returned -- %d\n");
        DEBUG_PRINT_P(loggingMsg, statusCode);
    }
    SelectPiccApp();
    memset(&Key, 0x00, sizeof(CryptoKeyBufferType));
    WriteAppKey(0x00, 0x00, Key, sizeof(CryptoKeyBufferType));
    SynchronizeAppDir();
}

void FactoryFormatPiccEV0(void) {
    /* Wipe PICC data */
    memset(&Picc, PICC_FORMAT_BYTE, sizeof(Picc));
    /* Set a random new UID */
    BYTE uidData[DESFIRE_UID_SIZE];
    RandomGetBuffer(uidData, DESFIRE_UID_SIZE);
    memcpy(&Picc.Uid[0], uidData, DESFIRE_UID_SIZE);
    /* Conform to NXP Application Note AN10927 about the first
     * byte of a randomly generated UID (refer to section 2.1.1).
     */
    Picc.Uid[0] = ISO14443A_UID0_RANDOM;
    /* Initialize params to look like EV0 */
    Picc.StorageSize = DESFIRE_STORAGE_SIZE_4K;
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
    /* Set a random new UID */
    BYTE uidData[DESFIRE_UID_SIZE];
    RandomGetBuffer(uidData, DESFIRE_UID_SIZE);
    memcpy(&Picc.Uid[0], uidData, DESFIRE_UID_SIZE);
    /* Initialize params to look like EV1 */
    Picc.StorageSize = StorageSize;
    Picc.HwVersionMajor = DESFIRE_HW_MAJOR_EV1;
    Picc.HwVersionMinor = DESFIRE_HW_MINOR_EV1;
    Picc.SwVersionMajor = DESFIRE_SW_MAJOR_EV1;
    Picc.SwVersionMinor = DESFIRE_SW_MINOR_EV1;
    /* Continue with user data initialization */
    SynchronizePICCInfo();
    FormatPicc();
}

void GetPiccUid(ConfigurationUidType Uid) {
    memcpy(Uid, Picc.Uid, DESFIRE_UID_SIZE);
}

void SetPiccUid(ConfigurationUidType Uid) {
    memcpy(Picc.Uid, Uid, DESFIRE_UID_SIZE);
    SynchronizePICCInfo();
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
