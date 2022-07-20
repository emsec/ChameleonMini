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
 * DESFireApplicationDirectory.h
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../MifareDESFire.h"
#include "DESFireApplicationDirectory.h"
#include "DESFirePICCControl.h"
#include "DESFireCrypto.h"
#include "DESFireStatusCodes.h"
#include "DESFireInstructions.h"
#include "DESFireMemoryOperations.h"
#include "DESFireUtils.h"
#include "DESFireLogging.h"

/*
 * Global card structure support routines
 */

void SynchronizeAppDir(void) {
    WriteBlockBytes(&AppDir, DESFIRE_APP_DIR_BLOCK_ID, sizeof(DESFireAppDirType));
}

BYTE PMKConfigurationChangeable(void) {
    BYTE pmkSettings = ReadKeySettings(DESFIRE_PICC_APP_SLOT, DESFIRE_MASTER_KEY_ID);
    BYTE pmkPropMask = (0x01 << 3);
    return pmkSettings & pmkPropMask;
}

BYTE PMKRequiredForAppCreateDelete(void) {
    BYTE pmkSettings = ReadKeySettings(DESFIRE_PICC_APP_SLOT, DESFIRE_MASTER_KEY_ID);
    BYTE pmkPropMask = (0x01 << 2);
    return pmkSettings & pmkPropMask;
}

BYTE PMKFreeDirectoryListing(void) {
    BYTE pmkSettings = ReadKeySettings(DESFIRE_PICC_APP_SLOT, DESFIRE_MASTER_KEY_ID);
    BYTE pmkPropMask = (0x01 << 1);
    return pmkSettings & pmkPropMask;
}

BYTE PMKAllowChangingKey(void) {
    BYTE pmkSettings = ReadKeySettings(DESFIRE_PICC_APP_SLOT, DESFIRE_MASTER_KEY_ID);
    BYTE pmkPropMask = 0x01;
    return pmkSettings & pmkPropMask;
}

BYTE AMKConfigurationChangeable(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = (0x01 << 3);
    return amkSettings & amkPropMask;
}

BYTE AMKRequiredForFileCreateDelete(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = (0x01 << 2);
    return amkSettings & amkPropMask;
}

BYTE AMKFreeDirectoryListing(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = (0x01 << 1);
    return amkSettings & amkPropMask;
}

BYTE AMKAllowChangingKey(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = 0x01;
    return amkSettings & amkPropMask;
}

BYTE AMKRequiredToChangeKeys(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = 0xf0;
    return (amkSettings & amkPropMask) == 0x00;
}

BYTE AMKGetRequiredKeyToChangeKeys(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = 0xf0;
    BYTE authKeyId = (amkSettings & amkPropMask);
    if (0x10 <= authKeyId && authKeyId <= 0xd0) {
        return authKeyId;
    }
    return 0x00;
}

BYTE AMKRequireCurrentKeyToChangeKey(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = 0xf0;
    return (amkSettings & amkPropMask) == 0xe0;
}

BYTE AMKAllKeysFrozen(void) {
    BYTE amkSettings = ReadKeySettings(SelectedApp.Slot, DESFIRE_MASTER_KEY_ID);
    BYTE amkPropMask = 0xf0;
    return (amkSettings & amkPropMask) == 0xf0;
}

SIZET GetAppProperty(DesfireCardLayout propId, BYTE AppSlot) {
    if (AppSlot >= AppDir.FirstFreeSlot || AppSlot >= DESFIRE_MAX_SLOTS) {
        return 0x00;
    }
    SelectedAppCacheType appCache;
    ReadBlockBytes(&appCache, AppDir.AppCacheStructBlockOffset[AppSlot], sizeof(SelectedAppCacheType));
    switch (propId) {
        case DESFIRE_APP_KEY_COUNT:
            return appCache.KeyCount;
        case DESFIRE_APP_MAX_KEY_COUNT:
            return appCache.MaxKeyCount;
        case DESFIRE_APP_FILE_COUNT:
            return appCache.FileCount;
        case DESFIRE_APP_CRYPTO_COMM_STANDARD:
            return appCache.CryptoCommStandard;
        case DESFIRE_APP_KEY_SETTINGS_BLOCK_ID:
            return appCache.KeySettings;
        case DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID:
            return appCache.FileNumbersArrayMap;
        case DESFIRE_APP_FILE_COMM_SETTINGS_BLOCK_ID:
            return appCache.FileCommSettings;
        case DESFIRE_APP_FILE_ACCESS_RIGHTS_BLOCK_ID:
            return appCache.FileAccessRights;
        case DESFIRE_APP_KEY_VERSIONS_ARRAY_BLOCK_ID:
            return appCache.KeyVersionsArray;
        case DESFIRE_APP_KEY_TYPES_ARRAY_BLOCK_ID:
            return appCache.KeyTypesArray;
        case DESFIRE_APP_FILES_PTR_BLOCK_ID:
            return appCache.FilesAddress;
        case DESFIRE_APP_KEYS_PTR_BLOCK_ID:
            return appCache.KeyAddress;
        default:
            return 0x00;
    }
}

void SetAppProperty(DesfireCardLayout propId, BYTE AppSlot, SIZET Value) {
    if (AppSlot >= AppDir.FirstFreeSlot || AppSlot >= DESFIRE_MAX_SLOTS) {
        return;
    }
    SelectedAppCacheType appCache;
    ReadBlockBytes(&appCache, AppDir.AppCacheStructBlockOffset[AppSlot], sizeof(SelectedAppCacheType));
    switch (propId) {
        case DESFIRE_APP_KEY_COUNT:
            appCache.KeyCount = ExtractLSBBE(Value);
            break;
        case DESFIRE_APP_FILE_COUNT:
            appCache.FileCount = ExtractLSBBE(Value);
            break;
        case DESFIRE_APP_CRYPTO_COMM_STANDARD:
            appCache.CryptoCommStandard = ExtractLSBBE(Value);
            break;
        case DESFIRE_APP_KEY_SETTINGS_BLOCK_ID:
            appCache.KeySettings = Value;
            break;
        case DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID:
            appCache.FileNumbersArrayMap = Value;
            break;
        case DESFIRE_APP_FILE_COMM_SETTINGS_BLOCK_ID:
            appCache.FileCommSettings = Value;
            break;
        case DESFIRE_APP_FILE_ACCESS_RIGHTS_BLOCK_ID:
            appCache.FileAccessRights = Value;
            break;
        case DESFIRE_APP_KEY_VERSIONS_ARRAY_BLOCK_ID:
            appCache.KeyVersionsArray = Value;
            break;
        case DESFIRE_APP_KEY_TYPES_ARRAY_BLOCK_ID:
            appCache.KeyTypesArray = Value;
            break;
        case DESFIRE_APP_FILES_PTR_BLOCK_ID:
            appCache.FilesAddress = Value;
            break;
        case DESFIRE_APP_KEYS_PTR_BLOCK_ID:
            appCache.KeyAddress = Value;
            break;
        default:
            return;
    }
    WriteBlockBytes(&appCache, AppDir.AppCacheStructBlockOffset[AppSlot], sizeof(SelectedAppCacheType));
}

/*
 * Application key management
 */

bool KeyIdValid(uint8_t AppSlot, uint8_t KeyId) {
    if (KeyId >= DESFIRE_MAX_KEYS || KeyId >= ReadMaxKeyCount(AppSlot)) {
        const char *debugMsg = PSTR("INVKEY-KeyId(%02x)-RdMax(%02x)");
        DEBUG_PRINT_P(debugMsg, KeyId, ReadMaxKeyCount(AppSlot));
        return false;
    }
    return true;
}

BYTE ReadMaxKeyCount(uint8_t AppSlot) {
    return GetAppProperty(DESFIRE_APP_MAX_KEY_COUNT, AppSlot);
}

BYTE ReadKeyCount(uint8_t AppSlot) {
    return GetAppProperty(DESFIRE_APP_KEY_COUNT, AppSlot);
}

void WriteKeyCount(uint8_t AppSlot, BYTE KeyCount) {
    SetAppProperty(DESFIRE_APP_KEY_COUNT, AppSlot, (SIZET) KeyCount);
}

BYTE ReadKeySettings(uint8_t AppSlot, uint8_t KeyId) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || KeyId >= DESFIRE_MAX_KEYS) {
        return 0x00;
    }
    SIZET keySettingsBlockId = GetAppProperty(DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot);
    BYTE keySettingsArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keySettingsArray, keySettingsBlockId, DESFIRE_MAX_KEYS);
    return keySettingsArray[KeyId];
}

void WriteKeySettings(uint8_t AppSlot, uint8_t KeyId, BYTE Value) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || KeyId >= DESFIRE_MAX_KEYS) {
        return;
    }
    SIZET keySettingsBlockId = GetAppProperty(DESFIRE_APP_KEY_SETTINGS_BLOCK_ID, AppSlot);
    BYTE keySettingsArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keySettingsArray, keySettingsBlockId, DESFIRE_MAX_KEYS);
    keySettingsArray[KeyId] = Value;
    WriteBlockBytes(keySettingsArray, keySettingsBlockId, DESFIRE_MAX_KEYS);
}

BYTE ReadKeyVersion(uint8_t AppSlot, uint8_t KeyId) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || KeyId >= DESFIRE_MAX_KEYS) {
        return 0x00;
    }
    SIZET keyVersionsBlockId = GetAppProperty(DESFIRE_APP_KEY_VERSIONS_ARRAY_BLOCK_ID, AppSlot);
    BYTE keyVersionsArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keyVersionsArray, keyVersionsBlockId, DESFIRE_MAX_KEYS);
    return keyVersionsArray[KeyId];
}

void WriteKeyVersion(uint8_t AppSlot, uint8_t KeyId, BYTE Value) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || KeyId >= DESFIRE_MAX_KEYS) {
        return;
    }
    SIZET keyVersionsBlockId = GetAppProperty(DESFIRE_APP_KEY_VERSIONS_ARRAY_BLOCK_ID, AppSlot);
    BYTE keyVersionsArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keyVersionsArray, keyVersionsBlockId, DESFIRE_MAX_KEYS);
    keyVersionsArray[KeyId] = Value;
    WriteBlockBytes(keyVersionsArray, keyVersionsBlockId, DESFIRE_MAX_KEYS);
}

BYTE ReadKeyCryptoType(uint8_t AppSlot, uint8_t KeyId) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || !KeyIdValid(AppSlot, KeyId)) {
        return 0x00;
    }
    SIZET keyTypesBlockId = GetAppProperty(DESFIRE_APP_KEY_TYPES_ARRAY_BLOCK_ID, AppSlot);
    BYTE keyTypesArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keyTypesArray, keyTypesBlockId, DESFIRE_MAX_KEYS);
    return keyTypesArray[KeyId];
}

void WriteKeyCryptoType(uint8_t AppSlot, uint8_t KeyId, BYTE Value) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || !KeyIdValid(AppSlot, KeyId)) {
        return 0x00;
    }
    SIZET keyTypesBlockId = GetAppProperty(DESFIRE_APP_KEY_TYPES_ARRAY_BLOCK_ID, AppSlot);
    BYTE keyTypesArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keyTypesArray, keyTypesBlockId, DESFIRE_MAX_KEYS);
    keyTypesArray[KeyId] = Value;
    WriteBlockBytes(keyTypesArray, keyTypesBlockId, DESFIRE_MAX_KEYS);
}

SIZET ReadKeyStorageAddress(uint8_t AppSlot) {
    return GetAppProperty(DESFIRE_APP_KEYS_PTR_BLOCK_ID, AppSlot);
}

void WriteKeyStorageAddress(uint8_t AppSlot, SIZET Value) {
    SetAppProperty(DESFIRE_APP_KEYS_PTR_BLOCK_ID, AppSlot, Value);
}

void ReadAppKey(uint8_t AppSlot, uint8_t KeyId, uint8_t *Key, SIZET KeySize) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || !KeyIdValid(AppSlot, KeyId)) {
        return;
    } else if (KeySize > CRYPTO_MAX_KEY_SIZE) {
        return;
    }
    SIZET keyStorageArrayBlockId = ReadKeyStorageAddress(AppSlot);
    SIZET keyStorageArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keyStorageArray, keyStorageArrayBlockId, 2 * DESFIRE_MAX_KEYS);
    ReadBlockBytes(Key, keyStorageArray[KeyId], KeySize);
}

void WriteAppKey(uint8_t AppSlot, uint8_t KeyId, const uint8_t *Key, SIZET KeySize) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || KeyId >= DESFIRE_MAX_KEYS) {
        return;
    } else if (KeySize > CRYPTO_MAX_KEY_SIZE) {
        return;
    }
    SIZET keyStorageArrayBlockId = ReadKeyStorageAddress(AppSlot);
    SIZET keyStorageArray[DESFIRE_MAX_KEYS];
    ReadBlockBytes(keyStorageArray, keyStorageArrayBlockId, 2 * DESFIRE_MAX_KEYS);
    WriteBlockBytes(Key, keyStorageArray[KeyId], KeySize);
}

/*
 * Application file management
 */

BYTE ReadFileCount(uint8_t AppSlot) {
    return (BYTE) GetAppProperty(DESFIRE_APP_FILE_COUNT, AppSlot);
}

void WriteFileCount(uint8_t AppSlot, BYTE FileCount) {
    SetAppProperty(DESFIRE_APP_FILE_COUNT, AppSlot, (SIZET) FileCount);
}

BYTE LookupFileNumberIndex(uint8_t AppSlot, BYTE FileNumber) {
    if (AppSlot >= DESFIRE_MAX_SLOTS) {
        return DESFIRE_MAX_FILES;
    }
    SIZET fileNumbersHashmapBlockId = GetAppProperty(DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID, AppSlot);
    BYTE fileNumbersHashmap[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileNumbersHashmap, fileNumbersHashmapBlockId, DESFIRE_MAX_FILES);
    BYTE fileIndex;
    for (fileIndex = 0; fileIndex < DESFIRE_MAX_FILES; fileIndex++) {
        if (fileNumbersHashmap[fileIndex] == FileNumber) {
            break;
        }
    }
    return fileIndex;
}

BYTE LookupFileNumberByIndex(uint8_t AppSlot, BYTE FileIndex) {
    if (AppSlot >= DESFIRE_MAX_SLOTS) {
        return DESFIRE_MAX_FILES;
    } else if (FileIndex >= DESFIRE_MAX_FILES) {
        return DESFIRE_MAX_FILES;
    }
    SIZET fileNumbersHashmapBlockId = GetAppProperty(DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID, AppSlot);
    BYTE fileNumbersHashmap[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileNumbersHashmap, fileNumbersHashmapBlockId, DESFIRE_MAX_FILES);
    return fileNumbersHashmap[FileIndex];
}

BYTE LookupNextFreeFileSlot(uint8_t AppSlot) {
    if (AppSlot >= DESFIRE_MAX_SLOTS) {
        return;
    }
    SIZET fileNumbersHashmapBlockId = GetAppProperty(DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID, AppSlot);
    BYTE fileNumbersHashmap[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileNumbersHashmap, fileNumbersHashmapBlockId, DESFIRE_MAX_FILES);
    uint8_t nextFreeSlot;
    for (nextFreeSlot = 0; nextFreeSlot < DESFIRE_MAX_FILES; ++nextFreeSlot) {
        if (fileNumbersHashmap[nextFreeSlot] == DESFIRE_FILE_NOFILE_INDEX) {
            return nextFreeSlot;
        }
    }
    return nextFreeSlot;
}

void WriteFileNumberAtIndex(uint8_t AppSlot, uint8_t FileIndex, BYTE FileNumber) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return;
    }
    SIZET fileNumbersHashmapBlockId = GetAppProperty(DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID, AppSlot);
    BYTE fileNumbersHashmap[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileNumbersHashmap, fileNumbersHashmapBlockId, DESFIRE_MAX_FILES);
    fileNumbersHashmap[FileIndex] = FileNumber;
    WriteBlockBytes(fileNumbersHashmap, fileNumbersHashmapBlockId, DESFIRE_MAX_FILES);
}

SIZET ReadFileDataStructAddress(uint8_t AppSlot, uint8_t FileIndex) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return 0;
    }
    SIZET filesAddressBlockId = GetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, SelectedApp.Slot);
    SIZET fileAddressArray[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileAddressArray, filesAddressBlockId, 2 * DESFIRE_MAX_FILES);
    return fileAddressArray[FileIndex];
}

uint8_t ReadFileType(uint8_t AppSlot, uint8_t FileIndex) {
    SIZET fileStructAddr = ReadFileDataStructAddress(AppSlot, FileIndex);
    if (fileStructAddr == 0) {
        return 0xff;
    }
    DESFireFileTypeSettings fileStorageData;
    ReadBlockBytes(&fileStorageData, fileStructAddr, sizeof(DESFireFileTypeSettings));
    return fileStorageData.FileType;
}

uint16_t ReadDataFileSize(uint8_t AppSlot, uint8_t FileIndex) {
    SIZET fileStructAddr = ReadFileDataStructAddress(AppSlot, FileIndex);
    if (fileStructAddr == 0) {
        return 0xffff;
    }
    DESFireFileTypeSettings fileStorageData;
    ReadBlockBytes(&fileStorageData, fileStructAddr, sizeof(DESFireFileTypeSettings));
    return fileStorageData.FileSize;
}

BYTE ReadFileCommSettings(uint8_t AppSlot, uint8_t FileIndex) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return 0x00;
    }
    SIZET fileCommSettingsBlockId = GetAppProperty(DESFIRE_APP_FILE_COMM_SETTINGS_BLOCK_ID, AppSlot);
    BYTE fileCommSettingsArray[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileCommSettingsArray, fileCommSettingsBlockId, DESFIRE_MAX_FILES);
    return fileCommSettingsArray[FileIndex];
}

void WriteFileCommSettings(uint8_t AppSlot, uint8_t FileIndex, BYTE CommSettings) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return;
    }
    SIZET fileCommSettingsBlockId = GetAppProperty(DESFIRE_APP_FILE_COMM_SETTINGS_BLOCK_ID, AppSlot);
    BYTE fileCommSettingsArray[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileCommSettingsArray, fileCommSettingsBlockId, DESFIRE_MAX_FILES);
    fileCommSettingsArray[FileIndex] = CommSettings;
    WriteBlockBytes(fileCommSettingsArray, fileCommSettingsBlockId, DESFIRE_MAX_FILES);
}

SIZET ReadFileAccessRights(uint8_t AppSlot, uint8_t FileIndex) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return 0x0000;
    }
    SIZET fileAccessRightsBlockId = GetAppProperty(DESFIRE_APP_FILE_ACCESS_RIGHTS_BLOCK_ID, AppSlot);
    SIZET fileAccessRightsArray[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileAccessRightsArray, fileAccessRightsBlockId, 2 * DESFIRE_MAX_FILES);
    return fileAccessRightsArray[FileIndex];
}

void WriteFileAccessRights(uint8_t AppSlot, uint8_t FileIndex, SIZET AccessRights) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return;
    }
    SIZET fileAccessRightsBlockId = GetAppProperty(DESFIRE_APP_FILE_ACCESS_RIGHTS_BLOCK_ID, AppSlot);
    SIZET fileAccessRightsArray[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileAccessRightsArray, fileAccessRightsBlockId, 2 * DESFIRE_MAX_FILES);
    fileAccessRightsArray[FileIndex] = AccessRights;
    WriteBlockBytes(fileAccessRightsArray, fileAccessRightsBlockId, 2 * DESFIRE_MAX_FILES);
}

DESFireFileTypeSettings ReadFileSettings(uint8_t AppSlot, uint8_t FileIndex) {
    DESFireFileTypeSettings fileTypeSettings = { 0 };
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return fileTypeSettings;
    }
    SIZET fileTypeSettingsBlockId = GetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot);
    SIZET fileTypeSettingsAddresses[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileTypeSettingsAddresses, fileTypeSettingsBlockId, 2 * DESFIRE_MAX_FILES);
    ReadBlockBytes(&fileTypeSettings, fileTypeSettingsAddresses[FileIndex], sizeof(DESFireFileTypeSettings));
    return fileTypeSettings;
}

void WriteFileSettings(uint8_t AppSlot, uint8_t FileIndex, DESFireFileTypeSettings *FileSettings) {
    if (AppSlot >= DESFIRE_MAX_SLOTS || FileIndex >= DESFIRE_MAX_FILES) {
        return;
    } else if (FileSettings == NULL) {
        return;
    }
    SIZET fileTypeSettingsBlockId = GetAppProperty(DESFIRE_APP_FILES_PTR_BLOCK_ID, AppSlot);
    SIZET fileTypeSettingsAddresses[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileTypeSettingsAddresses, fileTypeSettingsBlockId, 2 * DESFIRE_MAX_FILES);
    WriteBlockBytes(FileSettings, fileTypeSettingsAddresses[FileIndex], sizeof(DESFireFileTypeSettings));
    memcpy(&(SelectedFile.File), FileSettings, sizeof(DESFireFileTypeSettings));
}

/*
 * Application selection
 */

uint8_t LookupAppSlot(const DESFireAidType Aid) {
    uint8_t Slot;
    for (Slot = 0; Slot < DESFIRE_MAX_SLOTS; ++Slot) {
        if (!memcmp(AppDir.AppIds[Slot], Aid, DESFIRE_AID_SIZE)) {
            return Slot;
        }
    }
    return DESFIRE_MAX_SLOTS;
}

void SelectAppBySlot(uint8_t AppSlot) {
    if (AppSlot == SelectedApp.Slot) {
        return;
    }
    SIZET appCacheSelectedBlockId = AppDir.AppCacheStructBlockOffset[AppSlot];
    if (appCacheSelectedBlockId == 0) {
        return;
    }
    if (SelectedApp.Slot != (uint8_t) -1) {
        SIZET prevAppCacheSelectedBlockId = AppDir.AppCacheStructBlockOffset[SelectedApp.Slot];
        WriteBlockBytes(&SelectedApp, prevAppCacheSelectedBlockId, sizeof(SelectedAppCacheType));
    }
    ReadBlockBytes(&SelectedApp, appCacheSelectedBlockId, sizeof(SelectedAppCacheType));
    SelectedApp.Slot = AppSlot;
    SynchronizeAppDir();
}

bool GetAppData(uint8_t appSlot, SelectedAppCacheType *destData) {
    if (destData == NULL) {
        return false;
    }
    SIZET appCacheSelectedBlockId = AppDir.AppCacheStructBlockOffset[appSlot];
    if (appCacheSelectedBlockId == 0) {
        return false;
    }
    ReadBlockBytes(destData, appCacheSelectedBlockId, sizeof(SelectedAppCacheType));
    return true;
}

uint16_t SelectApp(const DESFireAidType Aid) {
    uint8_t Slot;
    /* Search for the app slot */
    Slot = LookupAppSlot(Aid);
    if (Slot >= DESFIRE_MAX_SLOTS) {
        return STATUS_APP_NOT_FOUND;
    }
    SelectAppBySlot(Slot);
    return STATUS_OPERATION_OK;
}

void SelectPiccApp(void) {
    SelectAppBySlot(DESFIRE_PICC_APP_SLOT);
}

bool IsPiccAppSelected(void) {
    return SelectedApp.Slot == DESFIRE_PICC_APP_SLOT;
}

/*
 * Application management
 */

uint16_t CreateApp(const DESFireAidType Aid, uint8_t KeyCount, uint8_t KeySettings) {
    uint8_t Slot;
    uint8_t FreeSlot;
    bool initMasterApp = false;

    /* Verify this AID has not been allocated yet */
    if ((Aid[0] == 0x00) && (Aid[1] == 0x00) && (Aid[2] == 0x00) && AppDir.FirstFreeSlot == 0) {
        Slot = 0;
        initMasterApp = true;
    } else if ((Aid[0] == 0x00) && (Aid[1] == 0x00) && (Aid[2] == 0x00)) {
        return STATUS_DUPLICATE_ERROR;
    } else if (LookupAppSlot(Aid) != DESFIRE_MAX_SLOTS) {
        return STATUS_DUPLICATE_ERROR;
    } else {
        Slot = AppDir.FirstFreeSlot;
    }
    /* Verify there is space */
    if (Slot >= DESFIRE_MAX_SLOTS) {
        return STATUS_APP_COUNT_ERROR;
    }
    /* Verify are not requesting more keys than the static capacity */
    if (KeyCount > DESFIRE_MAX_KEYS) {
        return STATUS_NO_SUCH_KEY;
    }
    /* Update the next free slot */
    for (FreeSlot = 1; FreeSlot < DESFIRE_MAX_SLOTS; ++FreeSlot) {
        if (FreeSlot != Slot && (AppDir.AppIds[FreeSlot][0] | AppDir.AppIds[FreeSlot][1] | AppDir.AppIds[FreeSlot][2]) == 0)
            break;
    }

    /* Allocate storage for the application structure itself */
    AppDir.AppCacheStructBlockOffset[Slot] = AllocateBlocks(SELECTED_APP_CACHE_TYPE_BLOCK_SIZE);
    if (AppDir.AppCacheStructBlockOffset[Slot] == 0) {
        const char *debugMsg = PSTR("X - alloc blks, slot = %d");
        DEBUG_PRINT_P(debugMsg, Slot);
        return STATUS_OUT_OF_EEPROM_ERROR;
    }
    /* Allocate storage for the application components */
    SelectedAppCacheType appCacheData;
    memset(&appCacheData, 0x00, sizeof(SelectedAppCacheType));
    appCacheData.Slot = Slot;
    appCacheData.KeyCount = 1; // Master Key
    appCacheData.MaxKeyCount = KeyCount;
    appCacheData.FileCount = 0;
    appCacheData.CryptoCommStandard = DesfireCommMode;
    appCacheData.KeySettings = AllocateBlocks(APP_CACHE_KEY_SETTINGS_ARRAY_BLOCK_SIZE);
    if (appCacheData.KeySettings == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        BYTE keySettings[DESFIRE_MAX_KEYS];
        memset(keySettings, KeySettings, DESFIRE_MAX_KEYS);
        keySettings[0] = KeySettings;
        WriteBlockBytes(keySettings, appCacheData.KeySettings, DESFIRE_MAX_KEYS);
    }
    appCacheData.FileNumbersArrayMap = AllocateBlocks(APP_CACHE_FILE_NUMBERS_HASHMAP_BLOCK_SIZE);
    if (appCacheData.FileNumbersArrayMap == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        BYTE fileNumbersArrayMapData[DESFIRE_MAX_FILES];
        memset(fileNumbersArrayMapData, DESFIRE_FILE_NOFILE_INDEX, DESFIRE_MAX_FILES);
        WriteBlockBytes(fileNumbersArrayMapData, appCacheData.FileNumbersArrayMap, DESFIRE_MAX_FILES);
    }
    appCacheData.FileCommSettings = AllocateBlocks(APP_CACHE_FILE_COMM_SETTINGS_ARRAY_BLOCK_SIZE);
    if (appCacheData.FileCommSettings == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        BYTE fileCommSettingsData[DESFIRE_MAX_FILES];
        memset(fileCommSettingsData, DESFIRE_DEFAULT_COMMS_STANDARD, DESFIRE_MAX_FILES);
        WriteBlockBytes(fileCommSettingsData, appCacheData.FileCommSettings, DESFIRE_MAX_FILES);
    }
    appCacheData.FileAccessRights = AllocateBlocks(APP_CACHE_FILE_ACCESS_RIGHTS_ARRAY_BLOCK_SIZE);
    if (appCacheData.FileAccessRights == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        SIZET fileAccessRightsData[DESFIRE_MAX_FILES];
        for (int fidx = 0; fidx < DESFIRE_MAX_FILES; fidx++) {
            fileAccessRightsData[fidx] = 0x000f;
        }
        WriteBlockBytes(fileAccessRightsData, appCacheData.FileAccessRights, sizeof(SIZET) * DESFIRE_MAX_FILES);
    }
    appCacheData.KeyVersionsArray = AllocateBlocks(APP_CACHE_KEY_VERSIONS_ARRAY_BLOCK_SIZE);
    if (appCacheData.KeyVersionsArray == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        BYTE keyVersionsData[DESFIRE_MAX_KEYS];
        memset(keyVersionsData, 0x00, DESFIRE_MAX_KEYS);
        WriteBlockBytes(keyVersionsData, appCacheData.KeyVersionsArray, DESFIRE_MAX_KEYS);
    }
    appCacheData.KeyTypesArray = AllocateBlocks(APP_CACHE_KEY_TYPES_ARRAY_BLOCK_SIZE);
    if (appCacheData.KeyTypesArray == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        BYTE keyTypesData[APP_CACHE_KEY_TYPES_ARRAY_BLOCK_SIZE * DESFIRE_BLOCK_SIZE];
        memset(keyTypesData, 0x00, APP_CACHE_KEY_TYPES_ARRAY_BLOCK_SIZE * DESFIRE_BLOCK_SIZE);
        WriteBlockBytes(keyTypesData, appCacheData.KeyTypesArray, APP_CACHE_KEY_TYPES_ARRAY_BLOCK_SIZE * DESFIRE_BLOCK_SIZE);
    }
    appCacheData.FilesAddress = AllocateBlocks(APP_CACHE_FILE_BLOCKIDS_ARRAY_BLOCK_SIZE);
    if (appCacheData.FilesAddress == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        SIZET fileAddressData[DESFIRE_MAX_FILES];
        memset(fileAddressData, 0x00, sizeof(SIZET) * DESFIRE_MAX_FILES);
        WriteBlockBytes(fileAddressData, appCacheData.FilesAddress, sizeof(SIZET) * DESFIRE_MAX_FILES);
    }
    appCacheData.KeyAddress = AllocateBlocks(APP_CACHE_KEY_BLOCKIDS_ARRAY_BLOCK_SIZE);
    if (appCacheData.KeyAddress == 0) {
        return STATUS_OUT_OF_EEPROM_ERROR;
    } else {
        SIZET keyAddresses[DESFIRE_MAX_KEYS];
        memset(keyAddresses, 0x00, sizeof(SIZET) * DESFIRE_MAX_KEYS);
        // Allocate the application Master Key:
        keyAddresses[0] = AllocateBlocks(APP_CACHE_MAX_KEY_BLOCK_SIZE);
        if (keyAddresses[0] == 0) {
            return STATUS_OUT_OF_EEPROM_ERROR;
        }
        BYTE cryptoBlankKeyData[CRYPTO_MAX_KEY_SIZE];
        memset(cryptoBlankKeyData, 0x00, CRYPTO_MAX_KEY_SIZE);
        WriteBlockBytes(cryptoBlankKeyData, keyAddresses[0], CRYPTO_MAX_KEY_SIZE);
        WriteBlockBytes(keyAddresses, appCacheData.KeyAddress, sizeof(SIZET) * DESFIRE_MAX_KEYS);
    }
    SIZET appCacheDataBlockId = AppDir.AppCacheStructBlockOffset[Slot];
    WriteBlockBytes(&appCacheData, appCacheDataBlockId, sizeof(SelectedAppCacheType));
    // Note: Creating the block DOES NOT mean we have selected it:
    if (initMasterApp) {
        memcpy(&SelectedApp, &appCacheData, sizeof(SelectedAppCacheType));
    }

    /* Update the directory */
    for (int aidx = 0; aidx < DESFIRE_AID_SIZE; aidx++) {
        AppDir.AppIds[Slot][aidx] = Aid[aidx];
    }
    AppDir.FirstFreeSlot = FreeSlot;
    SynchronizeAppDir();

    return STATUS_OPERATION_OK;
}

uint16_t DeleteApp(const DESFireAidType Aid) {
    uint8_t Slot;
    /* Search for the app slot */
    Slot = LookupAppSlot(Aid);
    if (Slot == DESFIRE_MAX_SLOTS) {
        return STATUS_APP_NOT_FOUND;
    }
    /* Deactivate the app */
    for (int aidx = 0; aidx < DESFIRE_AID_SIZE; aidx++) {
        AppDir.AppIds[Slot][aidx] = 0x00;
    }
    AppDir.FirstFreeSlot = MIN(Slot, AppDir.FirstFreeSlot);
    SynchronizeAppDir();
    if (!IsPiccAppSelected()) {
        InvalidateAuthState(0);
    }
    SelectAppBySlot(DESFIRE_PICC_APP_SLOT);
    return STATUS_OPERATION_OK;
}

void GetApplicationIdsSetup(void) {
    TransferState.GetApplicationIds.NextIndex = 0;
}

TransferStatus GetApplicationIdsTransfer(uint8_t *Buffer) {
    TransferStatus Status;
    uint8_t EntryIndex;
    Status.BytesProcessed = 0;
    for (EntryIndex = TransferState.GetApplicationIds.NextIndex;
            EntryIndex < DESFIRE_MAX_SLOTS; ++EntryIndex) {
        if ((AppDir.AppIds[EntryIndex][0] | AppDir.AppIds[EntryIndex][1] | AppDir.AppIds[EntryIndex][2]) == 0) {
            continue;
        }
        /* If it won't fit -- remember and return */
        if (Status.BytesProcessed >= TERMINAL_BUFFER_SIZE - 20) {
            TransferState.GetApplicationIds.NextIndex = EntryIndex;
            Status.IsComplete = false;
            return Status;
        }
        Buffer[Status.BytesProcessed++] = AppDir.AppIds[EntryIndex][0];
        Buffer[Status.BytesProcessed++] = AppDir.AppIds[EntryIndex][1];
        Buffer[Status.BytesProcessed++] = AppDir.AppIds[EntryIndex][2];
    }
    Status.IsComplete = true;
    return Status;
}

uint16_t GetApplicationIdsIterator(uint8_t *Buffer, uint16_t ByteCount) {
    TransferStatus Status;
    Status = GetApplicationIdsTransfer(&Buffer[1]);
    if (Status.IsComplete) {
        Buffer[0] = STATUS_OPERATION_OK;
        DesfireState = DESFIRE_IDLE;
    } else {
        Buffer[0] = STATUS_ADDITIONAL_FRAME;
        DesfireState = DESFIRE_GET_APPLICATION_IDS2;
    }
    return DESFIRE_STATUS_RESPONSE_SIZE + Status.BytesProcessed;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
