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
 * DESFirePICCHeaderLayout.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../../Common.h"

#include "DESFirePICCHeaderLayout.h"
#include "DESFirePICCControl.h"
#include "DESFireFile.h"
#include "DESFireLogging.h"
#include "DESFireStatusCodes.h"

SIZET PrettyPrintPICCHeaderData(BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    SIZET charsWritten = 0x00;
    charsWritten  = snprintf_P(outputBuffer, maxLength,
                               PSTR("(UID) %s\r\n"),
                               GetHexBytesString(Picc.Uid, DESFIRE_UID_SIZE));
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR("(VERSION) HW=%02x.%02x, SW=%02x.%02x\r\n"),
                               Picc.HwVersionMajor, Picc.HwVersionMinor,
                               Picc.SwVersionMajor, Picc.SwVersionMinor);
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR("(BATCH) %s\r\n"),
                               GetHexBytesString(Picc.BatchNumber, 5));
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR("(DATE) %02x/%02x\r\n"),
                               Picc.ProductionWeek, Picc.ProductionYear);
    BYTE atsBytes[6];
    memcpy(&atsBytes[0], Picc.ATSBytes, 5);
    atsBytes[5] = 0x80;
    BufferToHexString(__InternalStringBuffer, STRING_BUFFER_SIZE, atsBytes, 6);
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR("(ATS) %s\r\n"), __InternalStringBuffer);
    return charsWritten;
}

SIZET PrettyPrintFileContentsData(BYTE *outputBuffer, SIZET maxLength, BYTE fileNumber) {
    DESFireFileTypeSettings *fileData;
    if (ReadFileControlBlock(fileNumber, &SelectedFile) != STATUS_OPERATION_OK) {
        return 0;
    }
    fileData = &SelectedFile;
    switch (fileData->FileType) {
        case DESFIRE_FILE_STANDARD_DATA:
            return snprintf_P(outputBuffer, maxLength,
                              PSTR("    STD-DATA  @ % 3d bytes\r\n"),
                              fileData->FileSize);
        case DESFIRE_FILE_BACKUP_DATA:
            return snprintf_P(outputBuffer, maxLength,
                              PSTR("    BKUP-DATA @ % 3d bytes -- BlkCount = %d\r\n"),
                              fileData->FileSize, fileData->BackupFile.BlockCount);
        case DESFIRE_FILE_VALUE_DATA:
            return snprintf_P(outputBuffer, maxLength,
                              PSTR("    VALUE     @ Cln=%d|Dty=%d (PrevDebit=%d) in [%d, %d] with LimitedCredit=%c\r\n"),
                              fileData->ValueFile.CleanValue, fileData->ValueFile.DirtyValue, fileData->ValueFile.PreviousDebit,
                              fileData->ValueFile.LowerLimit, fileData->ValueFile.UpperLimit,
                              fileData->ValueFile.LimitedCreditEnabled == 0 ? 'N' : 'Y');
        case DESFIRE_FILE_LINEAR_RECORDS:
        case DESFIRE_FILE_CIRCULAR_RECORDS:
            return snprintf_P(outputBuffer, maxLength,
                              PSTR("    RECORD    @ [Type=%c] RcdSize=0x%06x|CurNumRcds=0x%06x|MaxRcds=0x%06x\r\n"),
                              fileData->FileType == DESFIRE_FILE_LINEAR_RECORDS ? 'L' : 'C',
                              GET_LE24(fileData->RecordFile.RecordSize),
                              GET_LE24(fileData->RecordFile.CurrentNumRecords),
                              GET_LE24(fileData->RecordFile.MaxRecordCount));
        default:
            break;
    }
    return 0;
}

SIZET PrettyPrintPICCFile(SelectedAppCacheType *appData, uint8_t fileIndex,
                          BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    BYTE charsWritten = 0x00;
    BYTE fileNumber = LookupFileNumberByIndex(appData->Slot, fileIndex);
    if (fileNumber >= DESFIRE_MAX_FILES) {
        return charsWritten;
    }
    if (verbose) {
        BYTE fileCommSettings = ReadFileCommSettings(appData->Slot, fileIndex);
        SIZET fileAccessRights = ReadFileAccessRights(appData->Slot, fileIndex);
        strncpy_P(__InternalStringBuffer2, GetCommSettingsDesc(fileCommSettings), DATA_BUFFER_SIZE_SMALL);
        __InternalStringBuffer2[DATA_BUFFER_SIZE_SMALL - 1] = '\0';
        charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                   PSTR("  -> No. %02x [Comm=%s -- Perms=%s]\r\n"),
                                   fileNumber, __InternalStringBuffer2,
                                   GetFileAccessPermissionsDesc(fileAccessRights));
        //charsWritten += PrettyPrintFileContentsData(outputBuffer + charsWritten, maxLength - charsWritten, fileIndex);
    } else {
        charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                   PSTR("  -> No. %02x\r\n"),
                                   fileNumber);
    }
    return charsWritten;
}

SIZET PrettyPrintPICCFilesFull(SelectedAppCacheType *appData, BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    SIZET charsWritten = 0x00;
    BYTE fileIndex;
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR(" [FILES -- %d of %d]\r\n"),
                               appData->FileCount, DESFIRE_MAX_FILES);
    uint8_t fileNumberIndexMap[DESFIRE_MAX_FILES];
    ReadBlockBytes(&fileNumberIndexMap, appData->FileNumbersArrayMap, DESFIRE_MAX_FILES);
    for (fileIndex = 0; fileIndex < DESFIRE_MAX_FILES; fileIndex++) {
        if (fileNumberIndexMap[fileIndex] == DESFIRE_FILE_NOFILE_INDEX) {
            continue;
        }
        charsWritten += PrettyPrintPICCFile(appData, fileIndex, outputBuffer + charsWritten,
                                            maxLength - charsWritten, verbose);
    }
    return charsWritten;
}

SIZET PrettyPrintPICCKey(SelectedAppCacheType *appData, uint8_t keyIndex,
                         BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    if (!KeyIdValid(appData->Slot, keyIndex)) {
        return 0x00;
    }
    BYTE charsWritten = 0x00;
    BYTE keySettings = ReadKeySettings(appData->Slot, keyIndex);
    BYTE keyVersion = ReadKeyVersion(appData->Slot, keyIndex);
    BYTE keyType = ReadKeyCryptoType(appData->Slot, keyIndex);
    strncpy_P(__InternalStringBuffer, GetCryptoMethodDesc(keyType), STRING_BUFFER_SIZE);
    __InternalStringBuffer[STRING_BUFFER_SIZE - 1] = '\0';
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR("  -> No. %02x (%s) [v% 2d"),
                               keyIndex, __InternalStringBuffer, keyVersion);
    if ((appData->Slot == DESFIRE_PICC_APP_SLOT) && (keyIndex == DESFIRE_MASTER_KEY_ID)) {
        charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                   PSTR(" -- PMK"));
        if (verbose) {
            charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                       PSTR(" -- S=%02x"), keySettings);
        }
    } else if (keyIndex == DESFIRE_MASTER_KEY_ID) {
        charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                   PSTR(" -- AMK"));
        if (verbose) {
            charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                       PSTR(" -- S=%02x"), keySettings);
        }
    } else if (verbose) {
        charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                   PSTR(" -- %02d"), keySettings);
    }
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR("]\r\n"));
    /*if(verbose) {
        uint8_t keySize = GetDefaultCryptoMethodKeySize(keyType);
        uint8_t keyData[keySize];
        ReadAppKey(appData->Slot, keyIndex, &keyData[0], keySize);
        charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                   PSTR("     KeyData @ % 2 bytes = "),
                                   keySize);
        charsWritten += BufferToHexString(outputBuffer + charsWritten, maxLength - charsWritten, keyData, keySize);
        charsWritten += snprintf(outputBuffer + charsWritten, maxLength - charsWritten, PSTR("\r\n"));
    }*/
    return charsWritten;
}

SIZET PrettyPrintPICCKeysFull(SelectedAppCacheType *appData, BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    SIZET charsWritten = 0x00;
    BYTE keyIndex;
    charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                               PSTR(" [KEYS -- %d of %d]\r\n"),
                               appData->KeyCount, appData->MaxKeyCount);
    uint16_t keyDataAddresses[DESFIRE_MAX_KEYS];
    ReadBlockBytes(&keyDataAddresses, ReadKeyStorageAddress(appData->Slot), 2 * DESFIRE_MAX_KEYS);
    for (keyIndex = 0; keyIndex < DESFIRE_MAX_KEYS; keyIndex++) {
        if (keyDataAddresses[keyIndex] == 0) {
            continue;
        }
        charsWritten += PrettyPrintPICCKey(appData, keyIndex, outputBuffer + charsWritten,
                                           maxLength - charsWritten, verbose);
    }
    return charsWritten;
}

SIZET PrettyPrintPICCAppDir(uint8_t appIndex, BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    SIZET charsWritten = 0x00;
    BYTE keyIndex, fileIndex;
    SelectedAppCacheType appData;
    if (!GetAppData(appIndex, &appData)) {
        return charsWritten;
    }
    appData.Slot = appIndex;
    charsWritten += PrettyPrintPICCKeysFull(&appData, outputBuffer + charsWritten,
                                            maxLength - charsWritten, verbose);
    if (appIndex == 0) { // master
        return charsWritten;
    }
    charsWritten += PrettyPrintPICCFilesFull(&appData, outputBuffer + charsWritten,
                                             maxLength - charsWritten, verbose);
    return charsWritten;
}

SIZET PrettyPrintPICCAppDirsFull(BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    SIZET charsWritten = 0x00;
    BYTE appDirIndex;
    for (appDirIndex = 0; appDirIndex < DESFIRE_MAX_SLOTS; appDirIndex++) {
        DESFireAidType curAID;
        memcpy(curAID, AppDir.AppIds[appDirIndex], MAX_AID_SIZE);
        if ((curAID[0] | curAID[1] | curAID[2]) == 0x00 && appDirIndex > 0) {
            continue;
        }
        charsWritten += snprintf_P(outputBuffer + charsWritten, maxLength - charsWritten,
                                   PSTR("== AID 0x%02x%02x%02x\r\n"),
                                   curAID[0], curAID[1], curAID[2]);
        charsWritten += PrettyPrintPICCAppDir(appDirIndex, outputBuffer + charsWritten,
                                              maxLength - charsWritten, verbose);
    }
    return charsWritten;
}

SIZET PrettyPrintPICCImageData(BYTE *outputBuffer, SIZET maxLength, BYTE verbose) {
    BYTE charsWritten = 0x00;
    charsWritten += PrettyPrintPICCHeaderData(outputBuffer + charsWritten, maxLength - charsWritten, verbose);
    charsWritten += PrettyPrintPICCAppDirsFull(outputBuffer + charsWritten, maxLength - charsWritten, verbose);
    outputBuffer[maxLength - 1] = '\0';
    return charsWritten;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
