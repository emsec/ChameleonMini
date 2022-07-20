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
 * DESFirePICCControl.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_PICC_CONTROL_H__
#define __DESFIRE_PICC_CONTROL_H__

#include "../../Configuration.h"

#include "DESFireFirmwareSettings.h"
#include "DESFirePICCHeaderLayout.h"
#include "DESFireInstructions.h"
#include "DESFireApplicationDirectory.h"
#include "DESFireFile.h"
#include "DESFireCrypto.h"

/*
 * Internal state variables:
 */

/* Cached data: flush to FRAM or relevant EEPROM addresses if changed */
extern DESFirePICCInfoType Picc;
extern DESFireAppDirType AppDir;

/* Cached app data */
extern SelectedAppCacheType SelectedApp;
extern SelectedFileCacheType SelectedFile;

typedef void (*TransferSourceFuncType)(BYTE *Buffer, BYTE Count);
typedef void (*TransferSinkFuncType)(BYTE *Buffer, BYTE Count);
typedef BYTE(*TransferEncryptFuncType)(BYTE *Buffer, BYTE Count);
typedef TransferStatus(*PiccToPcdTransferFilterFuncType)(BYTE *Buffer);
typedef BYTE(*PcdToPiccTransferFilterFuncType)(BYTE *Buffer, BYTE Count);

/* Stored transfer state for all transfers */
typedef union DESFIRE_FIRMWARE_PACKING {
    struct DESFIRE_FIRMWARE_ALIGNAT {
        BYTE NextIndex;
    } GetApplicationIds;
    BYTE BlockBuffer[CRYPTO_MAX_BLOCK_SIZE];
    struct DESFIRE_FIRMWARE_ALIGNAT {
        SIZET BytesLeft;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            TransferSourceFuncType Func;
            SIZET Pointer; /* in FRAM */
        } Source;
    } ReadData;
    struct DESFIRE_FIRMWARE_ALIGNAT {
        SIZET BytesLeft;
        struct DESFIRE_FIRMWARE_ALIGNAT {
            TransferSinkFuncType Func;
            SIZET Pointer; /* in FRAM */
        } Sink;
    } WriteData;
} TransferStateType;
extern TransferStateType TransferState;

/* Transfer routines */
void SyncronizePICCInfo(void);
TransferStatus PiccToPcdTransfer(uint8_t *Buffer);
uint8_t PcdToPiccTransfer(uint8_t *Buffer, uint8_t Count);

/* Setup routines */
uint8_t ReadDataFilterSetup(uint8_t CommSettings);
uint8_t WriteDataFilterSetup(uint8_t CommSettings);

/* PICC management */
void FormatPicc(void);
void CreatePiccApp(void);

void InitialisePiccBackendEV0(uint8_t StorageSize, bool formatPICC);
void InitialisePiccBackendEV1(uint8_t StorageSize, bool formatPICC);
void InitialisePiccBackendEV2(uint8_t StorageSize, bool formatPICC);

void FactoryFormatPiccEV0(void);
void FactoryFormatPiccEV1(uint8_t StorageSize);
void FactoryFormatPiccEV2(uint8_t StorageSize);

void ResetPiccBackend(void);

bool IsEmulatingEV1(void);

void GetPiccHardwareVersionInfo(uint8_t *Buffer);
void GetPiccSoftwareVersionInfo(uint8_t *Buffer);
void GetPiccManufactureInfo(uint8_t *Buffer);
uint8_t GetPiccKeySettings(void);

void GetPiccUid(ConfigurationUidType Uid);
void SetPiccUid(ConfigurationUidType Uid);

#endif
