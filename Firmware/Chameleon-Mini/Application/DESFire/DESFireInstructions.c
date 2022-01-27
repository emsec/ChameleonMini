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
 * DESFireInstructions.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include <string.h>
#include <avr/pgmspace.h>

#include "../../Configuration.h"
#include "../../Memory.h"
#include "../../Common.h"
#include "../../Random.h"

#include "DESFireInstructions.h"
#include "DESFirePICCControl.h"
#include "DESFireCrypto.h"
#include "DESFireStatusCodes.h"
#include "DESFireLogging.h"
#include "DESFireUtils.h"
#include "DESFireMemoryOperations.h"
#include "../MifareDESFire.h"

DesfireSavedCommandStateType DesfireCommandState = { 0 };

const __flash DESFireCommand DESFireCommandSet[] = {
    {
        .insCode = CMD_AUTHENTICATE,
        .insDesc = (const __flash char[]) { "Authenicate_Legacy" },
        .insFunc = &EV0CmdAuthenticateLegacy1
    },
    {
        .insCode = CMD_AUTHENTICATE_ISO,
        .insDesc = (const __flash char[]) { "Authenticate_ISO" },
        .insFunc = &DesfireCmdAuthenticate3KTDEA1
    },
    {
        .insCode = CMD_AUTHENTICATE_AES,
        .insDesc = (const __flash char[]) { "Authenticate_AES" },
        .insFunc = &DesfireCmdAuthenticateAES1
    },
    {
        .insCode = CMD_AUTHENTICATE_EV2_FIRST,
        .insDesc = (const __flash char[]) { "Authenticate_AES_EV2_First" },
        .insFunc = NULL
    },
    {
        .insCode = CMD_AUTHENTICATE_EV2_NONFIRST,
        .insDesc = (const __flash char[]) { "Authenticate_AES_EV2_NonFirst" },
        .insFunc = NULL
    },
    {
        .insCode = CMD_CHANGE_KEY_SETTINGS,
        .insDesc = (const __flash char[]) { "Change_Key_Settings" },
        .insFunc = &EV0CmdChangeKeySettings
    },
    {
        .insCode = CMD_SET_CONFIGURATION,
        .insDesc = (const __flash char[]) { "Set_Configuration" },
        .insFunc = NULL //&DesfireCmdSetConfiguration
    },
    {
        .insCode = CMD_CHANGE_KEY,
        .insDesc = (const __flash char[]) { "Change_Key" },
        .insFunc = &EV0CmdChangeKey
    },
    {
        .insCode = CMD_GET_KEY_VERSION,
        .insDesc = (const __flash char[]) { "Get_Key_Version" },
        .insFunc = &DesfireCmdGetKeyVersion
    },
    {
        .insCode = CMD_CREATE_APPLICATION,
        .insDesc = (const __flash char[]) { "Create_Application" },
        .insFunc = &EV0CmdCreateApplication
    },
    {
        .insCode = CMD_DELETE_APPLICATION,
        .insDesc = (const __flash char[]) { "Delete_Application" },
        .insFunc = &EV0CmdDeleteApplication
    },
    {
        .insCode = CMD_GET_APPLICATION_IDS,
        .insDesc = (const __flash char[]) { "Get_Application_IDs" },
        .insFunc = &EV0CmdGetApplicationIds1
    },
    {
        .insCode = CMD_FREE_MEMORY,
        .insDesc = (const __flash char[]) { "Free_Memory" },
        .insFunc = &DesfireCmdFreeMemory
    },
    {
        .insCode = CMD_GET_DF_NAMES,
        .insDesc = (const __flash char[]) { "Get_DF_Names" },
        .insFunc = &DesfireCmdGetDFNames
    },
    {
        .insCode = CMD_GET_KEY_SETTINGS,
        .insDesc = (const __flash char[]) { "Get_Key_Settings" },
        .insFunc = &EV0CmdGetKeySettings
    },
    {
        .insCode = CMD_SELECT_APPLICATION,
        .insDesc = (const __flash char[]) { "Select_Application" },
        .insFunc = &EV0CmdSelectApplication
    },
    {
        .insCode = CMD_FORMAT_PICC,
        .insDesc = (const __flash char[]) { "Format_PICC" },
        .insFunc = &EV0CmdFormatPicc
    },
    {
        .insCode = CMD_GET_VERSION,
        .insDesc = (const __flash char[]) { "Get_Version" },
        .insFunc = &EV0CmdGetVersion1
    },
    {
        .insCode = CMD_GET_CARD_UID,
        .insDesc = (const __flash char[]) { "Get_Card_UID" },
        .insFunc = &DesfireCmdGetCardUID
    },
    {
        .insCode = CMD_GET_FILE_IDS,
        .insDesc = (const __flash char[]) { "Get_File_IDs" },
        .insFunc = &EV0CmdGetFileIds
    },
    {
        .insCode = CMD_GET_FILE_SETTINGS,
        .insDesc = (const __flash char[]) { "Get_File_Settings" },
        .insFunc = &EV0CmdGetFileSettings
    },
    {
        .insCode = CMD_CHANGE_FILE_SETTINGS,
        .insDesc = (const __flash char[]) { "Change_File_Settings" },
        .insFunc = &EV0CmdChangeFileSettings
    },
    {
        .insCode = CMD_CREATE_STDDATA_FILE,
        .insDesc = (const __flash char[]) { "Create_Data_File" },
        .insFunc = &EV0CmdCreateStandardDataFile
    },
    {
        .insCode = CMD_CREATE_BACKUPDATA_FILE,
        .insDesc = (const __flash char[]) { "Create_Backup_File" },
        .insFunc = &EV0CmdCreateBackupDataFile
    },
    {
        .insCode = CMD_CREATE_VALUE_FILE,
        .insDesc = (const __flash char[]) { "Create_Value_File" },
        .insFunc = &EV0CmdCreateValueFile
    },
    {
        .insCode = CMD_CREATE_LINEAR_RECORD_FILE,
        .insDesc = (const __flash char[]) { "Create_Linear_Record_File" },
        .insFunc = &EV0CmdCreateLinearRecordFile
    },
    {
        .insCode = CMD_CREATE_CYCLIC_RECORD_FILE,
        .insDesc = (const __flash char[]) { "Create_Cyclic_Record_File" },
        .insFunc = &EV0CmdCreateCyclicRecordFile
    },
    {
        .insCode = CMD_DELETE_FILE,
        .insDesc = (const __flash char[]) { "Delete_File" },
        .insFunc = &EV0CmdDeleteFile
    },
    {
        .insCode = CMD_GET_ISO_FILE_IDS,
        .insDesc = (const __flash char[]) { "Get_ISO_File_IDs" },
        .insFunc = &EV0CmdGetFileIds
    },
    {
        .insCode = CMD_READ_DATA,
        .insDesc = (const __flash char[]) { "Read_Data" },
        .insFunc = &EV0CmdReadData
    },
    {
        .insCode = CMD_WRITE_DATA,
        .insDesc = (const __flash char[]) { "Write_Data" },
        .insFunc = &EV0CmdWriteData
    },
    {
        .insCode = CMD_GET_VALUE,
        .insDesc = (const __flash char[]) { "Get_Value" },
        .insFunc = &EV0CmdGetValue
    },
    {
        .insCode = CMD_CREDIT,
        .insDesc = (const __flash char[]) { "Credit" },
        .insFunc = &EV0CmdCredit
    },
    {
        .insCode = CMD_DEBIT,
        .insDesc = (const __flash char[]) { "Debit" },
        .insFunc = &EV0CmdDebit
    },
    {
        .insCode = CMD_LIMITED_CREDIT,
        .insDesc = (const __flash char[]) { "Limited_Credit" },
        .insFunc = &EV0CmdLimitedCredit
    },
    {
        .insCode = CMD_WRITE_RECORD,
        .insDesc = (const __flash char[]) { "Write_Record" },
        .insFunc = &EV0CmdWriteRecord
    },
    {
        .insCode = CMD_READ_RECORDS,
        .insDesc = (const __flash char[]) { "Read_Records" },
        .insFunc = &EV0CmdReadRecords
    },
    {
        .insCode = CMD_CLEAR_RECORD_FILE,
        .insDesc = (const __flash char[]) { "Clear_Record_File" },
        .insFunc = &EV0CmdClearRecords
    },
    {
        .insCode = CMD_COMMIT_TRANSACTION,
        .insDesc = (const __flash char[]) { "Commit_Transaction" },
        .insFunc = &EV0CmdCommitTransaction
    },
    {
        .insCode = CMD_ABORT_TRANSACTION,
        .insDesc = (const __flash char[]) { "Abort_Transaction" },
        .insFunc = &EV0CmdAbortTransaction
    },
    {
        .insCode = CMD_ISO7816_SELECT,
        .insDesc = (const __flash char[]) { "ISO7816_Select" },
        .insFunc = &ISO7816CmdSelect
    },
    {
        .insCode = CMD_ISO7816_GET_CHALLENGE,
        .insDesc = (const __flash char[]) { "ISO7816_Get_Challenge" },
        .insFunc = &ISO7816CmdGetChallenge
    },
    {
        .insCode = CMD_ISO7816_EXTERNAL_AUTHENTICATE,
        .insDesc = (const __flash char[]) { "ISO7816_External_Authenticate" },
        .insFunc = &ISO7816CmdExternalAuthenticate
    },
    {
        .insCode = CMD_ISO7816_INTERNAL_AUTHENTICATE,
        .insDesc = (const __flash char[]) { "ISO7816_Internal_Authenticate" },
        .insFunc = &ISO7816CmdInternalAuthenticate
    },
    {
        .insCode = CMD_ISO7816_READ_BINARY,
        .insDesc = (const __flash char[]) { "ISO7816_Read_Binary" },
        .insFunc = &ISO7816CmdReadBinary
    },
    {
        .insCode = CMD_ISO7816_UPDATE_BINARY,
        .insDesc = (const __flash char[]) { "ISO7816_Update_Binary" },
        .insFunc = &ISO7816CmdUpdateBinary
    },
    {
        .insCode = CMD_ISO7816_READ_RECORDS,
        .insDesc = (const __flash char[]) { "ISO7816_Read_Records" },
        .insFunc = &ISO7816CmdReadRecords
    },
    {
        .insCode = CMD_ISO7816_APPEND_RECORD,
        .insDesc = (const __flash char[]) { "ISO7816_Append_Record" },
        .insFunc = &ISO7816CmdAppendRecord
    }
};

uint16_t CallInstructionHandler(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount == 0) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t callingInsCode = Buffer[0];
    uint32_t insLookupTableBuf = &DESFireCommandSet[0];
    uint8_t cmdSetLength = sizeof(DESFireCommandSet) / sizeof(DESFireCommand);
    uint8_t curInsIndex = 0;
    while (curInsIndex < cmdSetLength) {
        DESFireCommand dfCmd;
        memcpy_P(&dfCmd, insLookupTableBuf + curInsIndex * sizeof(DESFireCommand), sizeof(DESFireCommand));
        if (dfCmd.insCode == callingInsCode) {
            if (dfCmd.insFunc == NULL) {
                snprintf_P(__InternalStringBuffer, STRING_BUFFER_SIZE, PSTR("NOT IMPLEMENTED: %s!"), dfCmd.insDesc);
                __InternalStringBuffer[STRING_BUFFER_SIZE - 1] = '\0';
                uint8_t bufSize = StringLength(__InternalStringBuffer, STRING_BUFFER_SIZE);
                LogEntry(LOG_INFO_DESFIRE_DEBUGGING_OUTPUT, (void *) __InternalStringBuffer, bufSize);
                return CmdNotImplemented(Buffer, ByteCount);
            }
            return dfCmd.insFunc(Buffer, ByteCount);
        }
        curInsIndex += 1;
    }
    return ISO14443A_APP_NO_RESPONSE;
}

uint16_t ExitWithStatus(uint8_t *Buffer, uint8_t StatusCode, uint16_t DefaultReturnValue) {
    Buffer[0] = StatusCode;
    return DefaultReturnValue;
}

uint16_t CmdNotImplemented(uint8_t *Buffer, uint16_t ByteCount) {
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire general commands
 */

uint16_t EV0CmdGetVersion1(uint8_t *Buffer, uint16_t ByteCount) {
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = DESFIRE_MANUFACTURER_ID;
    Buffer[2] = DESFIRE_TYPE;
    Buffer[3] = DESFIRE_SUBTYPE;
    GetPiccHardwareVersionInfo(&Buffer[4]);
    Buffer[7] = DESFIRE_HW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION2;
    return DESFIRE_VERSION1_BYTES_PROCESSED;
}

uint16_t EV0CmdGetVersion2(uint8_t *Buffer, uint16_t ByteCount) {
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    Buffer[1] = DESFIRE_MANUFACTURER_ID;
    Buffer[2] = DESFIRE_TYPE;
    Buffer[3] = DESFIRE_SUBTYPE;
    GetPiccSoftwareVersionInfo(&Buffer[4]);
    Buffer[7] = DESFIRE_SW_PROTOCOL_TYPE;
    DesfireState = DESFIRE_GET_VERSION3;
    return DESFIRE_VERSION2_BYTES_PROCESSED;
}

uint16_t EV0CmdGetVersion3(uint8_t *Buffer, uint16_t ByteCount) {
    Buffer[0] = STATUS_OPERATION_OK;
    GetPiccManufactureInfo(&Buffer[1]);
    DesfireState = DESFIRE_IDLE;
    return DESFIRE_VERSION3_BYTES_PROCESSED;
}

uint16_t EV0CmdFormatPicc(uint8_t *Buffer, uint16_t ByteCount) {
    /* Require the PICC app to be selected */
    if (!IsPiccAppSelected()) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify authentication settings */
    if (!IsAuthenticated() || (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID)) {
        /* PICC master key authentication is always required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t uidBytes[ISO14443A_UID_SIZE_DOUBLE];
    memcpy(uidBytes, &Picc.Uid[0], ISO14443A_UID_SIZE_DOUBLE);
    if (IsPiccEV0(Picc)) {
        FactoryFormatPiccEV0();
    } else {
        FactoryFormatPiccEV1(Picc.StorageSize);
    }
    memcpy(&Picc.Uid[0], uidBytes, ISO14443A_UID_SIZE_DOUBLE);
    SynchronizePICCInfo();
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

uint16_t DesfireCmdGetCardUID(uint8_t *Buffer, uint16_t ByteCount) {
    Buffer[0] = STATUS_OPERATION_OK;
    memcpy(&Buffer[1], Picc.Uid, ISO14443A_UID_SIZE_DOUBLE);
    return 1 + ISO14443A_UID_SIZE_DOUBLE;
}

uint16_t DesfireCmdSetConfiguration(uint8_t *Buffer, uint16_t ByteCount) {
    return CmdNotImplemented(Buffer, ByteCount);
}

uint16_t DesfireCmdFreeMemory(uint8_t *Buffer, uint16_t ByteCount) {
    // Returns the amount of free space left on the tag in bytes
    // Note that this does not account for overhead needed to store
    // file structures, so that if N bytes are reported, the actual
    // practical working space is less than N:
    uint16_t cardCapacityBlocks = GetCardCapacityBlocks();
    cardCapacityBlocks -= Picc.FirstFreeBlock;
    uint16_t freeMemoryBytes = cardCapacityBlocks * DESFIRE_EEPROM_BLOCK_SIZE;
    Buffer[0] = (uint8_t)(freeMemoryBytes >> 8);
    Buffer[1] = (uint8_t)(freeMemoryBytes >> 0);
    Buffer[2] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2;
}

/*
 * DESFire key management commands
 */

uint16_t EV0CmdAuthenticateLegacy1(uint8_t *Buffer, uint16_t ByteCount) {
    BYTE KeyId, Status;
    BYTE keySize;
    BYTE **Key, **IVBuffer;

    /* Reset authentication state right away */
    InvalidateAuthState(SelectedApp.Slot == DESFIRE_PICC_APP_SLOT);
    /* Validate command length */
    if (ByteCount != 2) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate number of keys: less than max */
    KeyId = Buffer[1];
    if (!KeyIdValid(SelectedApp.Slot, KeyId)) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Make sure that this key is AES, and figure out its byte size */
    BYTE cryptoKeyType = ReadKeyCryptoType(SelectedApp.Slot, KeyId);
    if (!CryptoTypeDES(cryptoKeyType)) {
        Buffer[0] = STATUS_NO_SUCH_KEY;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Indicate that we are in AES key authentication land */
    keySize = GetDefaultCryptoMethodKeySize(CRYPTO_TYPE_DES);
    DesfireCommandState.KeyId = KeyId;
    DesfireCommandState.CryptoMethodType = CRYPTO_TYPE_3K3DES;
    DesfireCommandState.ActiveCommMode = GetCryptoMethodCommSettings(CRYPTO_TYPE_3K3DES);

    /* Fetch the key */
    ReadAppKey(SelectedApp.Slot, KeyId, *Key, keySize);
    LogEntry(LOG_APP_AUTH_KEY, (const void *) *Key, keySize);

    /* Generate the nonce B (RndB / Challenge response) */
    if (LocalTestingMode != 0) {
        RandomGetBuffer(DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    } else {
        /* Fixed nonce for testing */
        DesfireCommandState.RndB[0] = 0xCA;
        DesfireCommandState.RndB[1] = 0xFE;
        DesfireCommandState.RndB[2] = 0xBA;
        DesfireCommandState.RndB[3] = 0xBE;
        DesfireCommandState.RndB[4] = 0x00;
        DesfireCommandState.RndB[5] = 0x11;
        DesfireCommandState.RndB[6] = 0x22;
        DesfireCommandState.RndB[7] = 0x33;
    }
    LogEntry(LOG_APP_NONCE_B, DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);

    /* Encrypt RndB with the selected key and transfer it back to the PCD */
    uint8_t rndBPadded[2 * CRYPTO_CHALLENGE_RESPONSE_BYTES];
    memset(rndBPadded, 0x00, 2 * CRYPTO_CHALLENGE_RESPONSE_BYTES);
    memcpy(rndBPadded, DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    EncryptDESBuffer(CRYPTO_CHALLENGE_RESPONSE_BYTES, rndBPadded,
                     &Buffer[1], *Key);

    /* Scrub the key */
    memset(*Key, 0, keySize);

    /* Done */
    DesfireState = DESFIRE_LEGACY_AUTHENTICATE2;
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    return DESFIRE_STATUS_RESPONSE_SIZE + CRYPTO_CHALLENGE_RESPONSE_BYTES;
}

uint16_t EV0CmdAuthenticateLegacy2(uint8_t *Buffer, uint16_t ByteCount) {
    BYTE KeyId;
    BYTE cryptoKeyType, keySize;
    BYTE **Key, **IVBuffer;

    /* Set status for the next incoming command on error */
    DesfireState = DESFIRE_IDLE;
    /* Validate command length */
    if (ByteCount != 2 * CRYPTO_DES_BLOCK_SIZE + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Reset parameters for authentication from the first exchange */
    KeyId = DesfireCommandState.KeyId;
    cryptoKeyType = DesfireCommandState.CryptoMethodType;
    keySize = GetDefaultCryptoMethodKeySize(CRYPTO_TYPE_3K3DES);
    ReadAppKey(SelectedApp.Slot, KeyId, *Key, keySize);

    /* Decrypt the challenge sent back to get RndA and a shifted RndB */
    BYTE challengeRndAB[2 * CRYPTO_CHALLENGE_RESPONSE_BYTES];
    BYTE challengeRndA[CRYPTO_CHALLENGE_RESPONSE_BYTES];
    BYTE challengeRndB[CRYPTO_CHALLENGE_RESPONSE_BYTES];
    DecryptDESBuffer(2 * CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndAB,
                     &Buffer[1], *Key);
    RotateArrayRight(challengeRndAB + CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndB,
                     CRYPTO_CHALLENGE_RESPONSE_BYTES);
    memcpy(challengeRndA, challengeRndAB, CRYPTO_CHALLENGE_RESPONSE_BYTES);

    /* Check that the returned RndB matches what we sent in the previous round */
    if (memcmp(DesfireCommandState.RndB, challengeRndB, CRYPTO_CHALLENGE_RESPONSE_BYTES)) {
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Authenticated successfully */
    Authenticated = 0x01;
    AuthenticatedWithKey = KeyId;
    AuthenticatedWithPICCMasterKey = (SelectedApp.Slot == DESFIRE_PICC_APP_SLOT) &&
                                     (KeyId == DESFIRE_MASTER_KEY_ID);

    /* Encrypt and send back the once rotated RndA buffer to the PCD */
    RotateArrayLeft(challengeRndA, challengeRndAB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    //memset(challengeRndAB, 0x00, 2 * CRYPTO_CHALLENGE_RESPONSE_BYTES);
    //memcpy(challengeRndAB, challengeRndA, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    EncryptDESBuffer(CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndAB,
                     &Buffer[1], *Key);

    /* Scrub the key */
    memset(*Key, 0, keySize);

    /* Return the status on success */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + CRYPTO_CHALLENGE_RESPONSE_BYTES;
}

uint16_t EV0CmdChangeKey(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t KeyId;
    uint8_t ChangeKeyId;
    uint8_t KeySettings;

    /* Validate command length */
    if ((ByteCount != 1 + 1 + CRYPTO_3KTDEA_KEY_SIZE) &&
            (ByteCount != 1 + 1 + CRYPTO_AES_KEY_SIZE)) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate number of keys, and make sure the KeyId is valid given the AID selected */
    KeyId = Buffer[1];
    if (!KeyIdValid(SelectedApp.Slot, KeyId)) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (IsPiccAppSelected() && KeyId != 0x00) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (!IsPiccAppSelected() && KeyId == 0x00) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Validate the state against change key settings */
    KeySettings = ReadKeySettings(SelectedApp.Slot, KeyId);
    ChangeKeyId = KeySettings >> 4;
    switch (ChangeKeyId) {
        case DESFIRE_ALL_KEYS_FROZEN:
            /* Only master key may be (potentially) changed */
            if (!IsAuthenticated() || (KeyId != DESFIRE_MASTER_KEY_ID) || !(KeySettings & DESFIRE_ALLOW_MASTER_KEY_CHANGE)) {
                Buffer[0] = STATUS_PERMISSION_DENIED;
                return DESFIRE_STATUS_RESPONSE_SIZE;
            }
            break;
        case DESFIRE_USE_TARGET_KEY:
            /* Authentication with the target key is required */
            if (!IsAuthenticated() || (KeyId != AuthenticatedWithKey)) {
                Buffer[0] = STATUS_PERMISSION_DENIED;
                return DESFIRE_STATUS_RESPONSE_SIZE;
            }
            break;
        default:
            /* Authentication with a specific key is required */
            if (!IsAuthenticated() || (KeyId != ChangeKeyId)) {
                Buffer[0] = STATUS_PERMISSION_DENIED;
                return DESFIRE_STATUS_RESPONSE_SIZE;
            }
            break;
    }

    /* Figure out the key size, and the crypto type from it: */
    uint8_t keySize = ByteCount - 2;
    uint8_t cryptoCommsTypeFromKey, cryptoType;
    if ((keySize != CRYPTO_3KTDEA_KEY_SIZE) && (keySize != CRYPTO_AES_KEY_SIZE)) {
        Buffer[0] = STATUS_NO_SUCH_KEY;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (keySize == CRYPTO_3KTDEA_KEY_SIZE) {
        cryptoCommsTypeFromKey = DESFIRE_COMMS_CIPHERTEXT_DES;
        cryptoType = CRYPTO_TYPE_3K3DES;
    } else {
        cryptoCommsTypeFromKey = DESFIRE_COMMS_CIPHERTEXT_AES128;
        cryptoType = CRYPTO_TYPE_AES128;
    }
    uint8_t nextKeyVersion = ReadKeyVersion(SelectedApp.Slot, KeyId) + 1;

    /* TODO: The PCD generates data differently based on whether AuthKeyId == ChangeKeyId */
    /* TODO: [NEED DOCS] NewKey^OldKey | CRC(NewKey^OldKey) | CRC(NewKey) | Padding */
    /* TODO: [NEED DOCS] NewKey | CRC(NewKey) | Padding */
    /* TODO: NOTE: Padding checks are skipped, because meh. */
    if (KeyId == AuthenticatedWithKey) {
        InvalidateAuthState(0x00);
    }

    /* Write the key, next version, and scrub */
    WriteAppKey(SelectedApp.Slot, KeyId, &Buffer[2], keySize);
    WriteKeyVersion(SelectedApp.Slot, KeyId, nextKeyVersion);
    WriteKeyCryptoType(SelectedApp.Slot, KeyId, cryptoType);
    memset(&Buffer[2], 0x00, keySize);

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

uint16_t EV0CmdGetKeySettings(uint8_t *Buffer, uint16_t ByteCount) {
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (!IsAuthenticated() && IsPiccAppSelected()) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (IsPiccAppSelected() && AuthenticatedWithKey != 0x00) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (!IsPiccAppSelected() && AuthenticatedWithKey == 0x00) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    Buffer[1] = ReadKeySettings(SelectedApp.Slot, AuthenticatedWithKey);
    Buffer[2] = DESFIRE_MAX_KEYS - 1;

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2;
}

uint16_t EV0CmdChangeKeySettings(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t NewSettings;

    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify whether settings are changeable */
    if (!Authenticated || !(ReadKeySettings(SelectedApp.Slot, AuthenticatedWithKey) & DESFIRE_ALLOW_CONFIG_CHANGE)) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify the master key has been authenticated with */
    if (!Authenticated || AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    NewSettings = Buffer[1];
    if (IsPiccAppSelected()) {
        NewSettings &= 0x0F;
    }
    WriteKeySettings(SelectedApp.Slot, AuthenticatedWithKey, NewSettings);

    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

uint16_t DesfireCmdGetKeyVersion(uint8_t *Buffer, uint16_t ByteCount) {
    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify whether settings are changeable */
    if (!Authenticated || !(ReadKeySettings(SelectedApp.Slot, AuthenticatedWithKey) & DESFIRE_ALLOW_CONFIG_CHANGE)) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify the master key has been authenticated with */
    if (IsPiccAppSelected() && AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (!IsPiccAppSelected() && AuthenticatedWithKey == DESFIRE_MASTER_KEY_ID) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify that the key is valid */
    uint8_t KeyId = Buffer[1];
    if (!KeyIdValid(SelectedApp.Slot, KeyId)) {
        Buffer[0] = STATUS_NO_SUCH_KEY;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Retrieve the key version */
    uint8_t keyVersion = ReadKeyVersion(SelectedApp.Slot, KeyId);
    Buffer[1] = keyVersion;
    /* Done */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + 1;
}

/*
 * DESFire application management commands
 */

uint16_t EV0CmdGetApplicationIds1(uint8_t *Buffer, uint16_t ByteCount) {
    /* Validate command length */
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Require the PICC app to be selected */
    if (!IsPiccAppSelected()) {
        Buffer[0] = STATUS_PERMISSION_DENIED;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Verify authentication settings */
    if ((ReadKeySettings(SelectedApp.Slot, AuthenticatedWithKey) & DESFIRE_FREE_DIRECTORY_LIST) &&
            (AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID)) {
        /* PICC master key authentication is required */
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Setup the job and jump to the worker routine */
    GetApplicationIdsSetup();
    return GetApplicationIdsIterator(Buffer, ByteCount);
}

uint16_t EV0CmdCreateApplication(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t KeyCount;
    uint8_t KeySettings;
    /* Validate command length */
    if (ByteCount != 1 + 3 + 1 + 1) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Require the PICC app to be selected */
    if (!IsPiccAppSelected()) {
        Status = STATUS_PERMISSION_DENIED;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    const DESFireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };
    KeySettings = Buffer[4];
    KeyCount = Buffer[5];
    /* Validate number of keys: less than max (one for the Master Key) */
    if (KeyCount > DESFIRE_MAX_KEYS || KeyCount == 0) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    if (PMKRequiredForAppCreateDelete() != 0x00 && (Authenticated == 0x00 || AuthenticatedWithKey != 0x00)) {
        /* PICC master key authentication is required */
        Status = STATUS_AUTHENTICATION_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Done */
    Status = CreateApp(Aid, KeyCount, KeySettings);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdDeleteApplication(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t PiccKeySettings;
    /* Validate command length */
    if (ByteCount != 1 + 3) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate AID: AID of all zeros cannot be deleted */
    const DESFireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };
    if ((Aid[0] | Aid[1] | Aid[2]) == 0x00) {
        Status = STATUS_PERMISSION_DENIED;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate authentication: a master key is always required */
    if (!Authenticated || AuthenticatedWithKey != DESFIRE_MASTER_KEY_ID) {
        Status = STATUS_AUTHENTICATION_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate authentication: deletion with PICC master key is always OK,
       but if another app is selected, have more permissions checking to do */
    if (!IsPiccAppSelected()) {
        /* Verify the selected application is the one being deleted */
        DESFireAidType selectedAID;
        memcpy(selectedAID, AppDir.AppIds[SelectedApp.Slot], 3);
        if (memcmp(selectedAID, Aid, 3)) {
            Buffer[0] = STATUS_PERMISSION_DENIED;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
        PiccKeySettings = GetPiccKeySettings();
        const char *logMsg = PSTR("PICC key settings -- %02x");
        DEBUG_PRINT_P(logMsg, PiccKeySettings);
        /* Check the PICC key settings whether it is OK to delete using app master key */
        if ((PiccKeySettings & DESFIRE_FREE_CREATE_DELETE) == 0x00) {
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        }
        SelectPiccApp();
        InvalidateAuthState(0x00);
    }
    /* Done */
    Status = DeleteApp(Aid);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdSelectApplication(uint8_t *Buffer, uint16_t ByteCount) {
    InvalidateAuthState(0x00);
    // handle a special case with EV1:
    // https://stackoverflow.com/questions/38232695/m4m-mifare-desfire-ev1-which-mifare-aid-needs-to-be-added-to-nfc-routing-table
    if (ByteCount == 8) {
        const uint8_t DesfireEV1SelectPICCAid[] = { 0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x00 };
        if (!memcmp(&Buffer[1], DesfireEV1SelectPICCAid, sizeof(DesfireEV1SelectPICCAid))) {
            SelectPiccApp();
            SynchronizeAppDir();
            Buffer[0] = STATUS_OPERATION_OK;
            return DESFIRE_STATUS_RESPONSE_SIZE;
        }
    }
    /* Validate command length */
    if (ByteCount != 1 + 3) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    const DESFireAidType Aid = { Buffer[1], Buffer[2], Buffer[3] };
    /* Done */
    Buffer[0] = SelectApp(Aid);
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

uint16_t DesfireCmdGetDFNames(uint8_t *Buffer, uint16_t ByteCount) {
    return CmdNotImplemented(Buffer, ByteCount);
}

/*
 * DESFire application file management commands
 */

uint16_t EV0CmdCreateStandardDataFile(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    __uint24 FileSize;
    /* Validate command length */
    if (ByteCount != 1 + 1 + 1 + 2 + 3) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Common args validation */
    FileNum = Buffer[1];
    CommSettings = Buffer[2];
    AccessRights = Buffer[3] | (Buffer[4] << 8);
    Status = CreateFileCommonValidation(FileNum);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    FileSize = GET_LE24(&Buffer[5]);
    Status = CreateStandardFile(FileNum, CommSettings, AccessRights, (uint16_t)FileSize);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdCreateBackupDataFile(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    __uint24 FileSize;

    /* Validate command length */
    if (ByteCount != 1 + 1 + 1 + 2 + 3) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Common args validation */
    FileNum = Buffer[1];
    CommSettings = Buffer[2];
    AccessRights = Buffer[3] | (Buffer[4] << 8);
    Status = CreateFileCommonValidation(FileNum);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    FileSize = GET_LE24(&Buffer[5]);
    Status = CreateBackupFile(FileNum, CommSettings, AccessRights, (uint16_t)FileSize);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdCreateValueFile(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    uint32_t LowerLimit, UpperLimit, Value;
    uint8_t LimitedCreditEnabled;
    /* Validate command length */
    if (ByteCount != 1 + 1 + 1 + 2 + 4 + 4 + 4 + 1) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Common args validation */
    FileNum = Buffer[1];
    Status = CreateFileCommonValidation(FileNum);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    CommSettings = Buffer[2];
    AccessRights = Buffer[3] | (Buffer[4] << 8);
    LowerLimit = GET_LE32(&Buffer[5]);
    UpperLimit = GET_LE32(&Buffer[9]);
    Value = GET_LE32(&Buffer[13]);
    LimitedCreditEnabled = Buffer[17];
    Status = CreateValueFile(FileNum, CommSettings, AccessRights, LowerLimit,
                             UpperLimit, Value, LimitedCreditEnabled);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdCreateLinearRecordFile(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount != 1 + 1 + 1 + 2 + 3 + 3) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t Status = CreateFileCommonValidation(fileNumber);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint8_t commSettings = Buffer[2];
    uint16_t accessRights = Buffer[3] | (Buffer[4] << 8);
    uint8_t *recordSizeBytes = &Buffer[5];
    uint8_t *maxRecordsBytes = &Buffer[8];
    uint16_t recordSize = recordSizeBytes[0] | (recordSizeBytes[1] << 8);
    uint16_t maxRecords = maxRecordsBytes[0] | (maxRecordsBytes[1] << 8);
    if (recordSize > maxRecords || maxRecords == 0) {
        Buffer[0] = STATUS_BOUNDARY_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileType = DESFIRE_FILE_LINEAR_RECORDS;
    Status = CreateRecordFile(fileType, fileNumber, commSettings, accessRights,
                              recordSizeBytes, maxRecordsBytes);
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

uint16_t EV0CmdCreateCyclicRecordFile(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount != 1 + 1 + 1 + 2 + 3 + 3) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t Status = CreateFileCommonValidation(fileNumber);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint8_t commSettings = Buffer[2];
    uint16_t accessRights = Buffer[3] | (Buffer[4] << 8);
    uint8_t *recordSizeBytes = &Buffer[5];
    uint8_t *maxRecordsBytes = &Buffer[8];
    __uint24 recordSize = GET_LE24(recordSizeBytes);
    __uint24 maxRecords = GET_LE24(maxRecordsBytes);
    if (recordSize > maxRecords || maxRecords == 0) {
        Buffer[0] = STATUS_BOUNDARY_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileType = DESFIRE_FILE_CIRCULAR_RECORDS;
    Status = CreateRecordFile(fileType, fileNumber, commSettings, accessRights,
                              recordSizeBytes, maxRecordsBytes);
    Buffer[0] = Status;
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

uint16_t EV0CmdDeleteFile(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t FileNum;
    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate file number */
    FileNum = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_FILE_NOT_FOUND;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate access settings */
    if (!Authenticated || !(ReadKeySettings(SelectedApp.Slot, AuthenticatedWithKey) & DESFIRE_FREE_CREATE_DELETE)) {
        Status = STATUS_AUTHENTICATION_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Need change permissions to delete the file */
    uint16_t fileAccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    switch (ValidateAuthentication(fileAccessRights, VALIDATE_ACCESS_CHANGE)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_PERMISSION_DENIED;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            ActiveCommMode = DESFIRE_COMMS_PLAINTEXT;
        /* Fall through */
        case VALIDATED_ACCESS_GRANTED:
            /* Carry on */
            break;
    }
    Status = DeleteFile(fileIndex);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdGetFileIds(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount != 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (!Authenticated || AuthenticatedWithKey != 0x00) {
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileIDs[DESFIRE_MAX_FILES];
    SIZET fileNumbersArrayMapBlockId = GetAppProperty(DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID, SelectedApp.Slot);
    ReadBlockBytes(fileIDs, fileNumbersArrayMapBlockId, DESFIRE_MAX_FILES);
    uint8_t *outputBufPtr = &Buffer[1];
    uint8_t activeFilesCount = 0x00;
    for (uint8_t slotNum = 0; slotNum < DESFIRE_MAX_FILES; slotNum++) {
        if (fileIDs[slotNum] != DESFIRE_FILE_NOFILE_INDEX) {
            *(outputBufPtr++) = fileIDs[slotNum];
            ++activeFilesCount;
        }
    }
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + activeFilesCount;
}

uint16_t EV0CmdGetFileSettings(uint8_t *Buffer, uint16_t ByteCount) {

    if (ByteCount != 1 + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    } else if (!Authenticated) {
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Buffer[0] = STATUS_FILE_NOT_FOUND;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    uint16_t accessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t Status;
    switch (ValidateAuthentication(accessRights, VALIDATE_ACCESS_READ | VALIDATE_ACCESS_READWRITE)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_PERMISSION_DENIED;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            ActiveCommMode = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    uint8_t appendedFileDataSize = 0x00;
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    uint8_t commSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    DESFireFileTypeSettings fileStorageData;
    ReadFileControlBlock(fileNumber, &fileStorageData);
    uint16_t fileSize = fileStorageData.FileSize;
    uint8_t *outBufPtr = &Buffer[1];
    if (fileType == DESFIRE_FILE_STANDARD_DATA ||
            fileType == DESFIRE_FILE_BACKUP_DATA) {
        outBufPtr[0] = fileType;
        outBufPtr[1] = commSettings;
        outBufPtr[2] = (uint8_t)(accessRights & 0x00ff);
        outBufPtr[3] = (uint8_t)((accessRights >> 8) & 0x00ff);
        outBufPtr[4] = (uint8_t)(fileSize & 0x00ff);
        outBufPtr[5] = (uint8_t)((fileSize >> 8) & 0x00ff);
        outBufPtr[6] = 0x00; // MSB of file size
        outBufPtr[7] = 0x00;
        appendedFileDataSize = 1 + 1 + 2 + 3;
    } else if (fileType == DESFIRE_FILE_VALUE_DATA) {
        outBufPtr[0] = fileType;
        outBufPtr[1] = commSettings;
        outBufPtr[2] = (uint8_t)(accessRights & 0x00ff);
        outBufPtr[3] = (uint8_t)((accessRights >> 8) & 0x00ff);
        Int32ToByteBuffer(outBufPtr + 4, fileStorageData.ValueFile.LowerLimit);
        Int32ToByteBuffer(outBufPtr + 8, fileStorageData.ValueFile.UpperLimit);
        Int32ToByteBuffer(outBufPtr + 12, fileStorageData.ValueFile.CleanValue);
        outBufPtr[16] = fileStorageData.ValueFile.LimitedCreditEnabled;
        appendedFileDataSize = 1 + 1 + 2 + 4 + 4 + 4 + 1;
    } else if (fileType == DESFIRE_FILE_LINEAR_RECORDS ||
               fileType == DESFIRE_FILE_CIRCULAR_RECORDS) {
        outBufPtr[0] = fileType;
        outBufPtr[1] = commSettings;
        outBufPtr[2] = (uint8_t)(accessRights & 0x00ff);
        outBufPtr[3] = (uint8_t)((accessRights >> 8) & 0x00ff);
        Int24ToByteBuffer(outBufPtr + 4, GET_LE24(fileStorageData.RecordFile.RecordSize));
        Int24ToByteBuffer(outBufPtr + 7, GET_LE24(fileStorageData.RecordFile.MaxRecordCount));
        Int24ToByteBuffer(outBufPtr + 10, GET_LE24(fileStorageData.RecordFile.CurrentNumRecords));
        appendedFileDataSize = 1 + 1 + 2 + 3 + 3 + 3;
    } else {
        Buffer[0] = STATUS_PICC_INTEGRITY_ERROR;
        return ;
    }
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + appendedFileDataSize;
}

uint16_t EV0CmdChangeFileSettings(uint8_t *Buffer, uint16_t ByteCount) {
    DESFireLogSourceCodeTODO("", GetSourceFileLoggingData());
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE; // TODO
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire data manipulation commands
 */

uint16_t EV0CmdReadData(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    __uint24 Offset;
    __uint24 Length;
    /* Validate command length */
    if (ByteCount != 1 + 1 + 3 + 3) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate file number */
    FileNum = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        /* Fall through */
        case VALIDATED_ACCESS_GRANTED:
            /* Carry on */
            break;
    }
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_STANDARD_DATA &&
            fileType != DESFIRE_FILE_BACKUP_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate offset and length (preliminary) */
    Offset = GET_LE24(&Buffer[2]);
    Length = GET_LE24(&Buffer[5]);
    uint16_t fileSize = ReadDataFileSize(SelectedApp.Slot, fileIndex);
    if ((Offset >= fileSize) || ((fileSize - Offset) < Length)) {
        Status = STATUS_BOUNDARY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Setup and start the transfer */
    Status = ReadDataFileSetup(fileIndex, CommSettings, (uint16_t) Offset, (uint16_t) Length);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    return ReadDataFileIterator(Buffer);
}

uint16_t EV0CmdWriteData(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    __uint24 Offset;
    __uint24 Length;

    /* Validate command length */
    if (ByteCount < 1 + 1 + 3 + 3) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    FileNum = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_WRITE)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_STANDARD_DATA &&
            fileType != DESFIRE_FILE_BACKUP_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate offset and length (preliminary) */
    Offset = GET_LE24(&Buffer[2]);
    Length = GET_LE24(&Buffer[5]);
    uint8_t dataBufLength = ByteCount - 8;
    if (dataBufLength < Length) { // TODO: Technically this can be extended with 0xaf
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint16_t fileSize = ReadDataFileSize(SelectedApp.Slot, fileIndex);
    if ((Offset >= fileSize) || ((fileSize - Offset) < Length)) {
        Status = STATUS_BOUNDARY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    } else if (Length + Offset >= TERMINAL_BUFFER_SIZE) {
        Status = STATUS_PICC_INTEGRITY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Shift the initial memory by an offset so we are not off when writing along
     * blocks to FRAM memory:
     */
    uint16_t dataWriteSize = ByteCount - 8;
    uint8_t *dataWriteBuffer = &Buffer[8];
    if (Offset > 0) {
        uint8_t precursorFileData[Offset];
        uint16_t fileDataStartAddr = GetFileDataAreaBlockId(fileIndex);
        ReadBlockBytes(precursorFileData, fileDataStartAddr, Offset);
        memmove(&Buffer[1] + Offset, &Buffer[8], dataWriteSize);
        memcpy(&Buffer[1], precursorFileData, Offset);
        dataWriteSize += Offset;
        dataWriteBuffer = &Buffer[1];
    }
    /* Setup and start the transfer */
    Status = WriteDataFileSetup(fileIndex, fileType, CommSettings, (uint16_t) Offset, (uint16_t) Length);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    Status = WriteDataFileIterator(dataWriteBuffer, dataWriteSize);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdGetValue(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    uint8_t FileNum;
    uint8_t CommSettings;
    uint16_t AccessRights;
    TransferStatus XferStatus;
    /* Validate command length */
    if (ByteCount != 1 + 1) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    FileNum = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, FileNum);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_VALUE_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Setup and start the transfer */
    /* TODO: Not currently using any encryption for transfer (see datasheet) */
    DESFireFileTypeSettings fileData;
    ReadFileControlBlock(FileNum, &fileData);
    Buffer[0] = STATUS_OPERATION_OK;
    Int32ToByteBuffer(Buffer + 1, fileData.ValueFile.CleanValue);
    return DESFIRE_STATUS_RESPONSE_SIZE + 4;
}

uint16_t EV0CmdCredit(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount != 1 + 1 + 4) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    int32_t creditAmount = Int32FromByteBuffer(&Buffer[2]);
    if (creditAmount < 0) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_VALUE_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    DESFireFileTypeSettings fileData;
    Status = ReadFileControlBlock(fileNumber, &fileData);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Make sure that the transaction will fit in the bounds of the tag */
    int32_t nextValueAmount = fileData.ValueFile.DirtyValue + creditAmount;
    if (nextValueAmount < fileData.ValueFile.LowerLimit ||
            nextValueAmount > fileData.ValueFile.UpperLimit) {
        Status = STATUS_BOUNDARY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    fileData.ValueFile.DirtyValue = nextValueAmount;
    Status = WriteFileControlBlock(fileNumber, &fileData);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdDebit(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount != 1 + 1 + 4) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    int32_t debitAmount = Int32FromByteBuffer(&Buffer[2]);
    if (debitAmount < 0) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(AccessRights,
                                   VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ | VALIDATE_ACCESS_WRITE)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_VALUE_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    DESFireFileTypeSettings fileData;
    Status = ReadFileControlBlock(fileNumber, &fileData);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Make sure that the transaction will fit in the bounds of the tag */
    int32_t nextValueAmount = fileData.ValueFile.DirtyValue - debitAmount;
    if (nextValueAmount < fileData.ValueFile.LowerLimit ||
            nextValueAmount > fileData.ValueFile.UpperLimit) {
        Status = STATUS_BOUNDARY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    fileData.ValueFile.DirtyValue = nextValueAmount;
    fileData.ValueFile.PreviousDebit -= debitAmount;
    Status = WriteFileControlBlock(fileNumber, &fileData);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdLimitedCredit(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount != 1 + 1 + 4) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    int32_t creditAmount = Int32FromByteBuffer(&Buffer[2]);
    if (creditAmount < 0) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_VALUE_DATA) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    DESFireFileTypeSettings fileData;
    Status = ReadFileControlBlock(fileNumber, &fileData);
    if (Status != STATUS_OPERATION_OK) {
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    if (fileData.ValueFile.LimitedCreditEnabled == 0) {
        Status = STATUS_PERMISSION_DENIED;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Make sure that the transaction will fit in the bounds of the tag */
    int32_t nextValueAmount = fileData.ValueFile.DirtyValue + creditAmount;
    if (nextValueAmount < fileData.ValueFile.LowerLimit ||
            nextValueAmount > fileData.ValueFile.UpperLimit) {
        Status = STATUS_BOUNDARY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    fileData.ValueFile.DirtyValue = nextValueAmount;
    Status = WriteFileControlBlock(fileNumber, &fileData);
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdReadRecords(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount != 1 + 1 + 3 + 3) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        const char *logMsg = PSTR("Invalid file index = %d / %d ; for FileNum = %d");
        DEBUG_PRINT_P(logMsg, fileIndex, DESFIRE_MAX_FILES, fileNumber);
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    __uint24 Offset = GET_LE24(&Buffer[2]);
    __uint24 Length = GET_LE24(&Buffer[5]);
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_LINEAR_RECORDS &&
            fileType != DESFIRE_FILE_CIRCULAR_RECORDS) {
        const char *logMsg = PSTR("Invalid file type = %d@%d");
        DEBUG_PRINT_P(logMsg, fileType, fileIndex);
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint16_t fileSize = ReadDataFileSize(SelectedApp.Slot, fileIndex);
    if ((Offset > fileSize) || (fileSize - Offset < Length)) {
        Status = STATUS_BOUNDARY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* We are only able to read chunks of data starting at a round block offset */
    uint8_t blockReadOffset = DESFIRE_BYTES_TO_BLOCKS(Offset);
    Status = ReadDataFileSetup(fileIndex, CommSettings, (uint16_t) blockReadOffset, (uint16_t) Length);
    if (Status != STATUS_OPERATION_OK) {
        const char *logMsg = PSTR("ReadDataFileSetup -- ERROR!");
        DEBUG_PRINT_P(logMsg);
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    return ReadDataFileIterator(Buffer);
}

uint16_t EV0CmdWriteRecord(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount < 1 + 1 + 3 + 3) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint8_t fileNumber = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    __uint24 Offset = GET_LE24(&Buffer[2]);
    __uint24 Length = GET_LE24(&Buffer[5]);
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    /* Verify authentication: read or read&write required */
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_WRITE)) {
        case VALIDATED_ACCESS_DENIED:
            Status = STATUS_AUTHENTICATION_ERROR;
            return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    /* Validate the file type */
    uint8_t fileType = ReadFileType(SelectedApp.Slot, fileIndex);
    if (fileType != DESFIRE_FILE_LINEAR_RECORDS &&
            fileType != DESFIRE_FILE_CIRCULAR_RECORDS) {
        Status = STATUS_PARAMETER_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Validate lengths and buffer sizes passed */
    uint16_t dataXferLength = ByteCount - 8;
    if (dataXferLength > Length) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    uint16_t fileSize = ReadDataFileSize(SelectedApp.Slot, fileIndex);
    if ((Offset >= fileSize) || (fileSize - Offset < Length)) {
        Status = STATUS_BOUNDARY_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* We are only able to read chunks of data starting at a round block offset */
    DesfireState = DESFIRE_WRITE_DATA_FILE;
    uint16_t offsetBlocks = DESFIRE_BYTES_TO_BLOCKS(Offset);
    if (offsetBlocks > 0) {
        --offsetBlocks;
    }
    uint16_t effectiveOffset = Offset % DESFIRE_EEPROM_BLOCK_SIZE;
    uint8_t dataWriteAddr = GetFileDataAreaBlockId(fileIndex) + offsetBlocks;
    memmove(&Buffer[effectiveOffset], &Buffer[8], dataXferLength);
    if (effectiveOffset > 0) {
        uint8_t priorFileData[effectiveOffset];
        ReadBlockBytes(priorFileData, dataWriteAddr, effectiveOffset);
        memcpy(&Buffer[0], priorFileData, effectiveOffset);
    }
    dataXferLength += effectiveOffset;
    WriteBlockBytes(&Buffer[0], dataWriteAddr, dataXferLength);
    TransferState.WriteData.Sink.Func = &WriteDataEEPROMSink;
    TransferState.WriteData.Sink.Pointer = dataWriteAddr + DESFIRE_BYTES_TO_BLOCKS(dataXferLength);
    Status = STATUS_OPERATION_OK;
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdClearRecords(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount != 1 + 1) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }



    DESFireLogSourceCodeTODO("", GetSourceFileLoggingData());
    Buffer[0] = STATUS_ILLEGAL_COMMAND_CODE; // TODO
    return DESFIRE_STATUS_RESPONSE_SIZE;
}

/*
 * DESFire transaction handling commands
 */

uint16_t EV0CmdCommitTransaction(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount != 1) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Loop over all of the value files (backup files?) in the currently
     * selected application, and update the uncommitted credit/debit
     * changes made since the last transaction was resolved.
     */
    uint16_t fileNumsArrayAddr = GetAppProperty(DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID, SelectedApp.Slot);
    uint8_t fileNumsByIndexArray[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileNumsByIndexArray, fileNumsArrayAddr, DESFIRE_MAX_FILES);
    Status = STATUS_OPERATION_OK;
    for (uint8_t fileIdx = 0; fileIdx < DESFIRE_MAX_FILES; ++fileIdx) {
        if (fileNumsByIndexArray[fileIdx] == DESFIRE_FILE_NOFILE_INDEX) {
            continue;
        } else if (ReadFileType(SelectedApp.Slot, fileIdx) != DESFIRE_FILE_VALUE_DATA) {
            continue;
        }
        DESFireFileTypeSettings fileData;
        Status = ReadFileControlBlock(fileNumsByIndexArray[fileIdx], &fileData);
        if (Status != STATUS_OPERATION_OK) {
            break;
        }
        fileData.ValueFile.CleanValue = fileData.ValueFile.DirtyValue;
        fileData.ValueFile.PreviousDebit = 0;
        Status = WriteFileControlBlock(fileNumsByIndexArray[fileIdx], &fileData);
        if (Status != STATUS_OPERATION_OK) {
            break;
        }
    }
    Picc.TransactionStarted = 0x00;
    SynchronizePICCInfo();
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

uint16_t EV0CmdAbortTransaction(uint8_t *Buffer, uint16_t ByteCount) {
    uint8_t Status;
    if (ByteCount != 1) {
        Status = STATUS_LENGTH_ERROR;
        return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
    }
    /* Loop over all of the value files (backup files?) in the currently
     * selected application, and remove/abort the uncommitted credit/debit
     * changes made since the last transaction was resolved.
     */
    uint16_t fileNumsArrayAddr = GetAppProperty(DESFIRE_APP_FILE_NUMBER_ARRAY_MAP_BLOCK_ID, SelectedApp.Slot);
    uint8_t fileNumsByIndexArray[DESFIRE_MAX_FILES];
    ReadBlockBytes(fileNumsByIndexArray, fileNumsArrayAddr, DESFIRE_MAX_FILES);
    Status = STATUS_OPERATION_OK;
    for (uint8_t fileIdx = 0; fileIdx < DESFIRE_MAX_FILES; ++fileIdx) {
        if (fileNumsByIndexArray[fileIdx] == DESFIRE_FILE_NOFILE_INDEX) {
            continue;
        } else if (ReadFileType(SelectedApp.Slot, fileIdx) != DESFIRE_FILE_VALUE_DATA) {
            continue;
        }
        DESFireFileTypeSettings fileData;
        Status = ReadFileControlBlock(fileNumsByIndexArray[fileIdx], &fileData);
        if (Status != STATUS_OPERATION_OK) {
            break;
        }
        fileData.ValueFile.DirtyValue = fileData.ValueFile.CleanValue;
        fileData.ValueFile.PreviousDebit = 0;
        Status = WriteFileControlBlock(fileNumsByIndexArray[fileIdx], &fileData);
        if (Status != STATUS_OPERATION_OK) {
            break;
        }
    }
    Picc.TransactionStarted = 0x00;
    SynchronizePICCInfo();
    return ExitWithStatus(Buffer, Status, DESFIRE_STATUS_RESPONSE_SIZE);
}

/*
 * EV1/EV2 supported commands
 */

uint16_t DesfireCmdAuthenticate3KTDEA1(uint8_t *Buffer, uint16_t ByteCount) {

    BYTE KeyId, Status;
    BYTE keySize;
    BYTE **Key, **IVBuffer;

    /* Reset authentication state right away */
    InvalidateAuthState(SelectedApp.Slot == DESFIRE_PICC_APP_SLOT);
    /* Validate command length */
    if (ByteCount != 2) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Validate number of keys: less than max */
    KeyId = Buffer[1];
    if (!KeyIdValid(SelectedApp.Slot, KeyId)) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Make sure that this key is AES, and figure out its byte size */
    BYTE cryptoKeyType = ReadKeyCryptoType(SelectedApp.Slot, KeyId);
    if (!CryptoType3KTDEA(cryptoKeyType)) {
        Buffer[0] = STATUS_NO_SUCH_KEY;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    InitAESCryptoKeyData(&AESCryptoSessionKey);
    InitAESCryptoKeyData(&AESCryptoIVBuffer);

    keySize = GetDefaultCryptoMethodKeySize(CRYPTO_TYPE_3K3DES);
    Key = &SessionKey;

    /* Indicate that we are in AES key authentication land */
    DesfireCommandState.KeyId = KeyId;
    DesfireCommandState.CryptoMethodType = CRYPTO_TYPE_3K3DES;
    DesfireCommandState.ActiveCommMode = GetCryptoMethodCommSettings(CRYPTO_TYPE_3K3DES);

    /* Fetch the key */
    ReadAppKey(SelectedApp.Slot, KeyId, *Key, keySize);
    LogEntry(LOG_APP_AUTH_KEY, (const void *) *Key, keySize);

    /* Generate the nonce B (RndB / Challenge response) */
    if (LocalTestingMode != 0) {
        RandomGetBuffer(DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    } else {
        /* Fixed nonce for testing */
        DesfireCommandState.RndB[0] = 0xCA;
        DesfireCommandState.RndB[1] = 0xFE;
        DesfireCommandState.RndB[2] = 0xBA;
        DesfireCommandState.RndB[3] = 0xBE;
        DesfireCommandState.RndB[4] = 0x00;
        DesfireCommandState.RndB[5] = 0x11;
        DesfireCommandState.RndB[6] = 0x22;
        DesfireCommandState.RndB[7] = 0x33;
    }
    LogEntry(LOG_APP_NONCE_B, DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);

    /* Encrypt RndB with the selected key and transfer it back to the PCD */
    uint8_t rndBPadded[CRYPTO_CHALLENGE_RESPONSE_BYTES];
    memset(rndBPadded, 0x00, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    memcpy(rndBPadded, DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    Encrypt3DESBuffer(CRYPTO_CHALLENGE_RESPONSE_BYTES, rndBPadded,
                      &Buffer[1], *Key);

    /* Scrub the key */
    memset(*Key, 0, keySize);

    /* Done */
    DesfireState = DESFIRE_ISO_AUTHENTICATE2;
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    return DESFIRE_STATUS_RESPONSE_SIZE + CRYPTO_CHALLENGE_RESPONSE_BYTES;

}

uint16_t DesfireCmdAuthenticate3KTDEA2(uint8_t *Buffer, uint16_t ByteCount) {
    BYTE KeyId;
    BYTE cryptoKeyType, keySize;
    BYTE **Key, **IVBuffer;

    /* Set status for the next incoming command on error */
    DesfireState = DESFIRE_IDLE;
    /* Validate command length */
    if (ByteCount != CRYPTO_AES_BLOCK_SIZE + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Reset parameters for authentication from the first exchange */
    KeyId = DesfireCommandState.KeyId;
    cryptoKeyType = DesfireCommandState.CryptoMethodType;
    keySize = GetDefaultCryptoMethodKeySize(CRYPTO_TYPE_3K3DES);
    ReadAppKey(SelectedApp.Slot, KeyId, *Key, keySize);

    /* Decrypt the challenge sent back to get RndA and a shifted RndB */
    BYTE challengeRndAB[2 * CRYPTO_CHALLENGE_RESPONSE_BYTES];
    BYTE challengeRndA[CRYPTO_CHALLENGE_RESPONSE_BYTES];
    BYTE challengeRndB[CRYPTO_CHALLENGE_RESPONSE_BYTES];
    Decrypt3DESBuffer(2 * CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndAB,
                      &Buffer[1], *Key);
    RotateArrayRight(challengeRndAB + CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndB,
                     CRYPTO_CHALLENGE_RESPONSE_BYTES);
    memcpy(challengeRndA, challengeRndAB, CRYPTO_CHALLENGE_RESPONSE_BYTES);

    /* Check that the returned RndB matches what we sent in the previous round */
    if (memcmp(DesfireCommandState.RndB, challengeRndB, CRYPTO_CHALLENGE_RESPONSE_BYTES)) {
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Authenticated successfully */
    Authenticated = 0x01;
    AuthenticatedWithKey = KeyId;
    AuthenticatedWithPICCMasterKey = (SelectedApp.Slot == DESFIRE_PICC_APP_SLOT) &&
                                     (KeyId == DESFIRE_MASTER_KEY_ID);

    /* Encrypt and send back the once rotated RndA buffer to the PCD */
    RotateArrayLeft(challengeRndA, challengeRndAB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    Encrypt3DESBuffer(CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndAB,
                      &Buffer[1], *Key);

    /* Scrub the key */
    memset(*Key, 0, keySize);

    /* Return the status on success */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + CRYPTO_CHALLENGE_RESPONSE_BYTES;

}

uint16_t DesfireCmdAuthenticateAES1(uint8_t *Buffer, uint16_t ByteCount) {

    BYTE KeyId, Status;
    BYTE keySize;
    BYTE **Key, **IVBuffer;

    /* Reset authentication state right away */
    InvalidateAuthState(SelectedApp.Slot == DESFIRE_PICC_APP_SLOT);
    /* Validate command length */
    if (ByteCount != 2) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    KeyId = Buffer[1];
    /* Validate number of keys: less than max */
    if (!KeyIdValid(SelectedApp.Slot, KeyId)) {
        Buffer[0] = STATUS_PARAMETER_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    /* Make sure that this key is AES, and figure out its byte size */
    BYTE cryptoKeyType = ReadKeyCryptoType(SelectedApp.Slot, KeyId);
    if (!CryptoTypeAES(cryptoKeyType)) {
        Buffer[0] = STATUS_NO_SUCH_KEY;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    InitAESCryptoKeyData(&AESCryptoSessionKey);
    InitAESCryptoKeyData(&AESCryptoIVBuffer);

    keySize = GetDefaultCryptoMethodKeySize(CRYPTO_TYPE_AES128);
    *Key = AESCryptoSessionKey;
    *IVBuffer = AESCryptoIVBuffer;

    /* Indicate that we are in AES key authentication land */
    DesfireCommandState.KeyId = KeyId;
    DesfireCommandState.CryptoMethodType = CRYPTO_TYPE_AES128;
    DesfireCommandState.ActiveCommMode = GetCryptoMethodCommSettings(CRYPTO_TYPE_AES128);

    /* Fetch the key */
    ReadAppKey(SelectedApp.Slot, KeyId, *Key, keySize);
    LogEntry(LOG_APP_AUTH_KEY, (const void *) *Key, keySize);
    CryptoAESGetConfigDefaults(&AESCryptoContext);
    CryptoAESInitContext(&AESCryptoContext);

    /* Generate the nonce B (RndB / Challenge response) */
    if (LocalTestingMode != 0) {
        RandomGetBuffer(&(DesfireCommandState.RndB[0]), CRYPTO_CHALLENGE_RESPONSE_BYTES);
    } else {
        /* Fixed nonce for testing */
        DesfireCommandState.RndB[0] = 0xCA;
        DesfireCommandState.RndB[1] = 0xFE;
        DesfireCommandState.RndB[2] = 0xBA;
        DesfireCommandState.RndB[3] = 0xBE;
        DesfireCommandState.RndB[4] = 0x00;
        DesfireCommandState.RndB[5] = 0x11;
        DesfireCommandState.RndB[6] = 0x22;
        DesfireCommandState.RndB[7] = 0x33;
    }
    //LogEntry(LOG_APP_NONCE_B, DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);

    /* Encrypt RndB with the selected key and transfer it back to the PCD */
    uint8_t rndBPadded[2 * CRYPTO_CHALLENGE_RESPONSE_BYTES];
    memset(rndBPadded, 0x00, 2 * CRYPTO_CHALLENGE_RESPONSE_BYTES);
    memcpy(rndBPadded, DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    Status = CryptoAESEncryptBuffer(2 * CRYPTO_CHALLENGE_RESPONSE_BYTES, rndBPadded, &Buffer[1], NULL, *Key);
    if (Status != STATUS_OPERATION_OK) {
        Buffer[0] = Status;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Scrub the key */
    memset(*Key, 0, keySize);

    /* Done */
    DesfireState = DESFIRE_AES_AUTHENTICATE2;
    Buffer[0] = STATUS_ADDITIONAL_FRAME;
    return DESFIRE_STATUS_RESPONSE_SIZE + 2 * CRYPTO_CHALLENGE_RESPONSE_BYTES;

}

uint16_t DesfireCmdAuthenticateAES2(uint8_t *Buffer, uint16_t ByteCount) {
    BYTE KeyId;
    BYTE cryptoKeyType, keySize;
    BYTE **Key, **IVBuffer;

    /* Set status for the next incoming command on error */
    DesfireState = DESFIRE_IDLE;
    /* Validate command length */
    if (ByteCount != CRYPTO_AES_BLOCK_SIZE + 1) {
        Buffer[0] = STATUS_LENGTH_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Reset parameters for authentication from the first exchange */
    keySize = GetDefaultCryptoMethodKeySize(CRYPTO_TYPE_AES128);
    KeyId = DesfireCommandState.KeyId;
    cryptoKeyType = DesfireCommandState.CryptoMethodType;
    ReadAppKey(SelectedApp.Slot, KeyId, *Key, keySize);

    /* Decrypt the challenge sent back to get RndA and a shifted RndB */
    BYTE challengeRndAB[2 * CRYPTO_CHALLENGE_RESPONSE_BYTES];
    BYTE challengeRndA[CRYPTO_CHALLENGE_RESPONSE_BYTES];
    BYTE challengeRndB[CRYPTO_CHALLENGE_RESPONSE_BYTES];
    CryptoAESDecryptBuffer(2 * CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndAB, &Buffer[1], NULL, *Key);
    RotateArrayRight(challengeRndAB + CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    memcpy(challengeRndA, challengeRndAB, CRYPTO_CHALLENGE_RESPONSE_BYTES);

    /* Check that the returned RndB matches what we sent in the previous round */
    if (memcmp(DesfireCommandState.RndB, challengeRndB, CRYPTO_CHALLENGE_RESPONSE_BYTES)) {
        memcpy(challengeRndAB, DesfireCommandState.RndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
        memcpy(challengeRndAB + CRYPTO_CHALLENGE_RESPONSE_BYTES, challengeRndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
        LogEntry(LOG_APP_NONCE_B, challengeRndAB, 2 * CRYPTO_CHALLENGE_RESPONSE_BYTES);
        Buffer[0] = STATUS_AUTHENTICATION_ERROR;
        return DESFIRE_STATUS_RESPONSE_SIZE;
    }

    /* Authenticated successfully */
    Authenticated = 0x01;
    AuthenticatedWithKey = KeyId;
    AuthenticatedWithPICCMasterKey = (SelectedApp.Slot == DESFIRE_PICC_APP_SLOT) &&
                                     (KeyId == DESFIRE_MASTER_KEY_ID);
    memcpy(SessionKey, challengeRndB, CRYPTO_CHALLENGE_RESPONSE_BYTES);

    /* Encrypt and send back the once rotated RndA buffer to the PCD */
    memset(challengeRndAB, 0x00, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    memcpy(challengeRndAB, challengeRndA, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    RotateArrayLeft(challengeRndA, challengeRndAB, CRYPTO_CHALLENGE_RESPONSE_BYTES);
    CryptoAESEncryptBuffer(CRYPTO_AES_BLOCK_SIZE, challengeRndAB, &Buffer[1], NULL, *Key);

    /* Scrub the key */
    memset(*Key, 0, keySize);

    /* Return the status on success */
    Buffer[0] = STATUS_OPERATION_OK;
    return DESFIRE_STATUS_RESPONSE_SIZE + CRYPTO_AES_BLOCK_SIZE;

}

uint16_t ISO7816CmdSelect(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount == 0) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816P1Data == ISO7816_UNSUPPORTED_MODE ||
               Iso7816P2Data == ISO7816_UNSUPPORTED_MODE) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816P1Data == ISO7816_SELECT_EF) {
        return ISO7816CmdSelectEF(Buffer, ByteCount);
    } else if (Iso7816P1Data == ISO7816_SELECT_DF) {
        return ISO7816CmdSelectDF(Buffer, ByteCount);
    }
    Buffer[0] = ISO7816_ERROR_SW1_INS_UNSUPPORTED;
    Buffer[1] = ISO7816_ERROR_SW2_INS_UNSUPPORTED;
    return ISO7816_STATUS_RESPONSE_SIZE;
}

uint16_t ISO7816CmdSelectEF(uint8_t *Buffer, uint16_t ByteCount) {
    Iso7816FileSelected = false;
    if (ByteCount != 1 + 1) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileNumberHandle = Buffer[1];
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumberHandle);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint8_t readFileStatus = ReadFileControlBlock(fileNumberHandle, &(SelectedFile.File));
    if (readFileStatus != STATUS_OPERATION_OK) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    Iso7816FileSelected = true;
    Buffer[0] = ISO7816_CMD_NO_ERROR;
    Buffer[1] = ISO7816_CMD_NO_ERROR;
    return ISO7816_STATUS_RESPONSE_SIZE;
}

uint16_t ISO7816CmdSelectDF(uint8_t *Buffer, uint16_t ByteCount) {
    InvalidateAuthState(0x00);
    if (ByteCount != 1 + DESFIRE_AID_SIZE) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    DESFireAidType incomingAid;
    memcpy(incomingAid, &Buffer[1], DESFIRE_AID_SIZE);
    uint8_t appSlot = LookupAppSlot(incomingAid);
    if (appSlot >= DESFIRE_MAX_SLOTS) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    SelectAppBySlot(appSlot);
    Buffer[0] = ISO7816_CMD_NO_ERROR;
    Buffer[1] = ISO7816_CMD_NO_ERROR;
    return ISO7816_STATUS_RESPONSE_SIZE;
}

uint16_t ISO7816CmdGetChallenge(uint8_t *Buffer, uint16_t ByteCount) {
    /* Reference:
     * https://cardwerk.com/smart-card-standard-iso7816-4-section-6-basic-interindustry-commands/#chap6_15
     * The maximum length of the expected challenge (in bytes) is provided in the Le field:
     * here, ByteCount - 1. We shall declare that this value needs to be at least 8 bytes, and
     * intend on returning only 8 bytes when it is set to larger.
     */
    if (ByteCount < 1 + 8) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_GET_CHALLENGE_ERROR_SW2_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    const uint8_t challengeRespDefaultBytes = 8;
    RandomGetBuffer(&Buffer[2], challengeRespDefaultBytes);
    // TODO: Should store this value somewhere for the next commands / auth routines ...
    Buffer[0] = ISO7816_CMD_NO_ERROR;
    Buffer[1] = ISO7816_CMD_NO_ERROR;
    return ISO7816_STATUS_RESPONSE_SIZE + challengeRespDefaultBytes;
}

uint16_t ISO7816CmdExternalAuthenticate(uint8_t *Buffer, uint16_t ByteCount) {
    return CmdNotImplemented(Buffer, ByteCount);
}

uint16_t ISO7816CmdInternalAuthenticate(uint8_t *Buffer, uint16_t ByteCount) {
    return CmdNotImplemented(Buffer, ByteCount);
}

uint16_t ISO7816CmdReadBinary(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount == 0) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint8_t maxBytesToRead = ByteCount - 1;
    if ((Iso7816EfIdNumber > ISO7816_EFID_NUMBER_MAX) && !Iso7816FileSelected) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_NOEF;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816EfIdNumber <= ISO7816_EFID_NUMBER_MAX) {
        uint8_t fileNumberHandle = Iso7816EfIdNumber;
        uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumberHandle);
        if (fileIndex >= DESFIRE_MAX_FILES) {
            Buffer[0] = ISO7816_ERROR_SW1;
            Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
            return ISO7816_STATUS_RESPONSE_SIZE;
        }
        uint8_t readFileStatus = ReadFileControlBlockIntoCacheStructure(fileNumberHandle, &SelectedFile);
        if (readFileStatus != STATUS_OPERATION_OK) {
            Buffer[0] = ISO7816_ERROR_SW1;
            Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
            return ISO7816_STATUS_RESPONSE_SIZE;
        }
        Iso7816FileSelected = true;
        Iso7816EfIdNumber = ISO7816_EF_NOT_SPECIFIED;
    }
    /* Verify authentication: read or read&write required */
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, SelectedFile.File.FileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ)) {
        case VALIDATED_ACCESS_DENIED:
            Buffer[0] = ISO7816_ERROR_SW1_ACCESS;
            Buffer[1] = ISO7816_ERROR_SW2_SECURITY;
            return ISO7816_STATUS_RESPONSE_SIZE;
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    if (SelectedFile.File.FileType == DESFIRE_FILE_STANDARD_DATA ||
            SelectedFile.File.FileType == DESFIRE_FILE_BACKUP_DATA) {
        Buffer[0] = ISO7816_ERROR_SW1_ACCESS;
        Buffer[1] = ISO7816_ERROR_SW2_INCOMPATFS;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    if (maxBytesToRead == ISO7816_READ_ALL_BYTES_SIZE && SelectedFile.File.FileSize > ISO7816_MAX_FILE_SIZE) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_INCORRECT_P1P2;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816FileOffset >= SelectedFile.File.FileSize) {
        Buffer[0] = ISO7816_ERROR_SW1_WRONG_FSPARAMS;
        Buffer[1] = ISO7816_ERROR_SW2_WRONG_FSPARAMS;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (SelectedFile.File.FileSize - Iso7816FileOffset < maxBytesToRead) {
        Buffer[0] = ISO7816_ERROR_SW1_FSE;
        Buffer[1] = ISO7816_ERROR_SW2_EOF;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    if (maxBytesToRead == ISO7816_READ_ALL_BYTES_SIZE) {
        maxBytesToRead = SelectedFile.File.FileSize - Iso7816FileOffset;
    }
    /* Handle fetching bits in the cases where the file offset is not a multiple of
     * DESFIRE_EEPROM_BLOCK_SIZE, which is required to address the data read out of the
     * file using ReadBlockBytes
     */
    uint8_t fileDataReadAddr = SelectedFile.File.FileDataAddress + MIN(0, DESFIRE_BYTES_TO_BLOCKS(Iso7816FileOffset) - 1);
    uint8_t *storeDataBufStart = &Buffer[2];
    if ((Iso7816FileOffset % DESFIRE_EEPROM_BLOCK_SIZE) != 0) {
        uint8_t blockPriorByteCount = Iso7816FileOffset % DESFIRE_EEPROM_BLOCK_SIZE;
        uint8_t blockData[DESFIRE_EEPROM_BLOCK_SIZE];
        ReadBlockBytes(blockData, fileDataReadAddr, DESFIRE_EEPROM_BLOCK_SIZE);
        memcpy(storeDataBufStart + blockPriorByteCount, blockData, DESFIRE_EEPROM_BLOCK_SIZE);
        fileDataReadAddr += 1;
        storeDataBufStart += DESFIRE_EEPROM_BLOCK_SIZE - blockPriorByteCount;
        maxBytesToRead -= DESFIRE_EEPROM_BLOCK_SIZE - blockPriorByteCount;
    }
    ReadBlockBytes(storeDataBufStart, fileDataReadAddr, maxBytesToRead);
    Buffer[0] = ISO7816_CMD_NO_ERROR;
    Buffer[1] = ISO7816_CMD_NO_ERROR;
    return ISO7816_STATUS_RESPONSE_SIZE + maxBytesToRead;
}

uint16_t ISO7816CmdUpdateBinary(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount < 1 + 1) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint8_t maxBytesToRead = ByteCount - 1;
    if ((Iso7816EfIdNumber > ISO7816_EFID_NUMBER_MAX) && !Iso7816FileSelected) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_NOEF;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816EfIdNumber <= ISO7816_EFID_NUMBER_MAX) {
        uint8_t fileNumberHandle = Iso7816EfIdNumber;
        uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumberHandle);
        if (fileIndex >= DESFIRE_MAX_FILES) {
            Buffer[0] = ISO7816_ERROR_SW1;
            Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
            return ISO7816_STATUS_RESPONSE_SIZE;
        }
        uint8_t readFileStatus = ReadFileControlBlockIntoCacheStructure(fileNumberHandle, &SelectedFile);
        if (readFileStatus != STATUS_OPERATION_OK) {
            Buffer[0] = ISO7816_ERROR_SW1;
            Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
            return ISO7816_STATUS_RESPONSE_SIZE;
        }
        Iso7816FileSelected = true;
        Iso7816EfIdNumber = ISO7816_EF_NOT_SPECIFIED;
    }
    /* Verify authentication: read or read&write required */
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, SelectedFile.File.FileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ)) {
        case VALIDATED_ACCESS_DENIED:
            Buffer[0] = ISO7816_ERROR_SW1_ACCESS;
            Buffer[1] = ISO7816_ERROR_SW2_SECURITY;
            return ISO7816_STATUS_RESPONSE_SIZE;
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    if (SelectedFile.File.FileType != DESFIRE_FILE_STANDARD_DATA &&
            SelectedFile.File.FileType != DESFIRE_FILE_BACKUP_DATA) {
        Buffer[0] = ISO7816_ERROR_SW1_ACCESS;
        Buffer[1] = ISO7816_ERROR_SW2_INCOMPATFS;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    if (maxBytesToRead == ISO7816_READ_ALL_BYTES_SIZE && SelectedFile.File.FileSize > ISO7816_MAX_FILE_SIZE) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_INCORRECT_P1P2;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816FileOffset >= SelectedFile.File.FileSize) {
        Buffer[0] = ISO7816_ERROR_SW1_WRONG_FSPARAMS;
        Buffer[1] = ISO7816_ERROR_SW2_WRONG_FSPARAMS;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (SelectedFile.File.FileSize - Iso7816FileOffset < maxBytesToRead) {
        Buffer[0] = ISO7816_ERROR_SW1_FSE;
        Buffer[1] = ISO7816_ERROR_SW2_EOF;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    if (maxBytesToRead == ISO7816_READ_ALL_BYTES_SIZE) {
        maxBytesToRead = SelectedFile.File.FileSize - Iso7816FileOffset;
    }
    /* Handle fetching bits in the cases where the file offset is not a multiple of
     * DESFIRE_EEPROM_BLOCK_SIZE, which is required to address the data read out of the
     * file using ReadBlockBytes
     */
    uint8_t fileDataWriteAddr = SelectedFile.File.FileDataAddress + MIN(0, DESFIRE_BYTES_TO_BLOCKS(Iso7816FileOffset) - 1);
    uint8_t *updateDataBufStart = &Buffer[1];
    if ((Iso7816FileOffset % DESFIRE_EEPROM_BLOCK_SIZE) != 0) {
        uint8_t blockPriorByteCount = Iso7816FileOffset % DESFIRE_EEPROM_BLOCK_SIZE;
        uint8_t blockData[DESFIRE_EEPROM_BLOCK_SIZE];
        ReadBlockBytes(blockData, fileDataWriteAddr, blockPriorByteCount);
        memcpy(blockData + blockPriorByteCount, updateDataBufStart, DESFIRE_EEPROM_BLOCK_SIZE - blockPriorByteCount);
        WriteBlockBytes(blockData, fileDataWriteAddr, DESFIRE_EEPROM_BLOCK_SIZE);
        fileDataWriteAddr += 1;
        updateDataBufStart += DESFIRE_EEPROM_BLOCK_SIZE - blockPriorByteCount;
        maxBytesToRead -= DESFIRE_EEPROM_BLOCK_SIZE - blockPriorByteCount;
    }
    WriteBlockBytes(updateDataBufStart, fileDataWriteAddr, maxBytesToRead);
    Buffer[0] = ISO7816_CMD_NO_ERROR;
    Buffer[1] = ISO7816_CMD_NO_ERROR;
    return ISO7816_STATUS_RESPONSE_SIZE + maxBytesToRead;
}

uint16_t ISO7816CmdReadRecords(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount == 0) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint8_t maxBytesToRead = ByteCount - 1;
    if ((Iso7816EfIdNumber > ISO7816_EFID_NUMBER_MAX) && !Iso7816FileSelected) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_NOEF;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816EfIdNumber <= ISO7816_EFID_NUMBER_MAX) {
        uint8_t fileNumberHandle = Iso7816EfIdNumber;
        uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, fileNumberHandle);
        if (fileIndex >= DESFIRE_MAX_FILES) {
            Buffer[0] = ISO7816_ERROR_SW1;
            Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
            return ISO7816_STATUS_RESPONSE_SIZE;
        }
        uint8_t readFileStatus = ReadFileControlBlockIntoCacheStructure(fileNumberHandle, &SelectedFile);
        if (readFileStatus != STATUS_OPERATION_OK) {
            Buffer[0] = ISO7816_ERROR_SW1;
            Buffer[1] = ISO7816_SELECT_ERROR_SW2_NOFILE;
            return ISO7816_STATUS_RESPONSE_SIZE;
        }
        Iso7816FileSelected = true;
        Iso7816EfIdNumber = ISO7816_EF_NOT_SPECIFIED;
    }
    /* Verify authentication: read or read&write required */
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, SelectedFile.File.FileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ)) {
        case VALIDATED_ACCESS_DENIED:
            Buffer[0] = ISO7816_ERROR_SW1_ACCESS;
            Buffer[1] = ISO7816_ERROR_SW2_SECURITY;
            return ISO7816_STATUS_RESPONSE_SIZE;
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    if (SelectedFile.File.FileType != DESFIRE_FILE_LINEAR_RECORDS &&
            SelectedFile.File.FileType != DESFIRE_FILE_CIRCULAR_RECORDS) {
        Buffer[0] = ISO7816_ERROR_SW1_ACCESS;
        Buffer[1] = ISO7816_ERROR_SW2_INCOMPATFS;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (((maxBytesToRead == ISO7816_READ_ALL_BYTES_SIZE) &&
                (SelectedFile.File.FileSize > ISO7816_MAX_FILE_SIZE)) ||
               ((maxBytesToRead != ISO7816_READ_ALL_BYTES_SIZE) && (SelectedFile.File.FileSize < maxBytesToRead))) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_INCORRECT_P1P2;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (Iso7816FileOffset >= SelectedFile.File.FileSize) {
        Buffer[0] = ISO7816_ERROR_SW1_WRONG_FSPARAMS;
        Buffer[1] = ISO7816_ERROR_SW2_WRONG_FSPARAMS;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    if (maxBytesToRead == ISO7816_READ_ALL_BYTES_SIZE) {
        maxBytesToRead = MIN(SelectedFile.File.FileSize - Iso7816FileOffset, ISO7816_MAX_FILE_SIZE);
    }
    uint8_t cyclicRecordOffsetDiff = 0;
    if ((SelectedFile.File.FileType == DESFIRE_FILE_CIRCULAR_RECORDS) &&
            (SelectedFile.File.FileSize - Iso7816FileOffset < maxBytesToRead)) {
        cyclicRecordOffsetDiff = maxBytesToRead + Iso7816FileOffset - SelectedFile.File.FileSize;
    }
    /* Handle fetching bits in the cases where the file offset is not a multiple of
     * DESFIRE_EEPROM_BLOCK_SIZE, which is required to address the data read out of the
     * file using ReadBlockBytes
     */
    uint8_t initFileReadDataAddr = SelectedFile.File.FileDataAddress + MIN(0, DESFIRE_BYTES_TO_BLOCKS(Iso7816FileOffset) - 1);
    uint8_t *storeDataBufStart = &Buffer[2];
    if ((Iso7816FileOffset % DESFIRE_EEPROM_BLOCK_SIZE) != 0) {
        uint8_t blockPriorByteCount = Iso7816FileOffset % DESFIRE_EEPROM_BLOCK_SIZE;
        uint8_t blockData[DESFIRE_EEPROM_BLOCK_SIZE];
        ReadBlockBytes(blockData, initFileReadDataAddr, DESFIRE_EEPROM_BLOCK_SIZE);
        memcpy(storeDataBufStart + blockPriorByteCount, blockData, DESFIRE_EEPROM_BLOCK_SIZE);
        initFileReadDataAddr += 1;
        storeDataBufStart += DESFIRE_EEPROM_BLOCK_SIZE - blockPriorByteCount;
        maxBytesToRead -= DESFIRE_EEPROM_BLOCK_SIZE - blockPriorByteCount;
    }
    /* Now, read the specified file contents into the Buffer:
     * Cases to handle separately:
     *      1) Cyclic record files with a non-zero offset difference;
     *      2) Any other record file cases.
     */
    if ((SelectedFile.File.FileType == DESFIRE_FILE_CIRCULAR_RECORDS) &&
            (cyclicRecordOffsetDiff > 0)) {
        uint8_t initEOFReadLength = maxBytesToRead - cyclicRecordOffsetDiff;
        ReadBlockBytes(&Buffer[2], initFileReadDataAddr, initEOFReadLength);
        ReadBlockBytes(&Buffer[initEOFReadLength + 2], SelectedFile.File.FileDataAddress, cyclicRecordOffsetDiff);
    } else {
        ReadBlockBytes(&Buffer[2], initFileReadDataAddr, maxBytesToRead);
    }
    Buffer[0] = ISO7816_CMD_NO_ERROR;
    Buffer[1] = ISO7816_CMD_NO_ERROR;
    return ISO7816_STATUS_RESPONSE_SIZE + maxBytesToRead;
}

/* TODO: Due to many corner cases, this ISO7816 command needs to get most thoroughly tested first */
uint16_t ISO7816CmdAppendRecord(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount < 1 + 1) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_SELECT_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if (!Iso7816FileSelected) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    } else if ((SelectedFile.File.FileType != DESFIRE_FILE_LINEAR_RECORDS) &&
               (SelectedFile.File.FileType != DESFIRE_FILE_CIRCULAR_RECORDS)) {
        Buffer[0] = ISO7816_ERROR_SW1_WRONG_FSPARAMS;
        Buffer[1] = ISO7816_ERROR_SW2_WRONG_FSPARAMS;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint8_t fileIndex = LookupFileNumberIndex(SelectedApp.Slot, SelectedFile.File.FileNumber);
    if (fileIndex >= DESFIRE_MAX_FILES) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FUNC_UNSUPPORTED;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint16_t AccessRights = ReadFileAccessRights(SelectedApp.Slot, fileIndex);
    uint8_t CommSettings = ReadFileCommSettings(SelectedApp.Slot, fileIndex);
    switch (ValidateAuthentication(AccessRights, VALIDATE_ACCESS_READWRITE | VALIDATE_ACCESS_READ)) {
        case VALIDATED_ACCESS_DENIED:
            Buffer[0] = ISO7816_ERROR_SW1_ACCESS;
            Buffer[1] = ISO7816_ERROR_SW2_SECURITY;
            return ISO7816_STATUS_RESPONSE_SIZE;
        case VALIDATED_ACCESS_GRANTED_PLAINTEXT:
            CommSettings = DESFIRE_COMMS_PLAINTEXT;
        case VALIDATED_ACCESS_GRANTED:
            break;
    }
    uint16_t appendRecordLength = ByteCount - 1;
    uint16_t fileMaxRecords = SelectedFile.File.RecordFile.MaxRecordCount[0] |
                              (SelectedFile.File.RecordFile.MaxRecordCount[1] << 8);
    if (((SelectedFile.File.FileType == DESFIRE_FILE_LINEAR_RECORDS) &&
            (SelectedFile.File.RecordFile.BlockCount + appendRecordLength >= fileMaxRecords)) ||
            (appendRecordLength > fileMaxRecords)) {
        Buffer[0] = ISO7816_ERROR_SW1;
        Buffer[1] = ISO7816_ERROR_SW2_FILE_NOMEM;
        return ISO7816_STATUS_RESPONSE_SIZE;
    }
    uint16_t nextRecordPointer = 0;
    if (SelectedFile.File.FileType == DESFIRE_FILE_LINEAR_RECORDS) {
        SelectedFile.File.RecordFile.BlockCount += appendRecordLength;
        nextRecordPointer = SelectedFile.File.RecordFile.BlockCount;
    } else if (SelectedFile.File.FileType == DESFIRE_FILE_CIRCULAR_RECORDS) {
        SelectedFile.File.RecordFile.BlockCount = MIN(SelectedFile.File.RecordFile.BlockCount + appendRecordLength,
                                                      fileMaxRecords - 1);
        nextRecordPointer = (SelectedFile.File.RecordFile.RecordPointer + appendRecordLength) % fileMaxRecords;
    }
    uint16_t nextRecordIndexToAppend = SelectedFile.File.RecordFile.RecordPointer % fileMaxRecords;
    uint16_t priorBlockBytesToCopy = DESFIRE_EEPROM_BLOCK_SIZE - (nextRecordIndexToAppend % DESFIRE_EEPROM_BLOCK_SIZE);
    if ((priorBlockBytesToCopy == DESFIRE_EEPROM_BLOCK_SIZE) || (nextRecordIndexToAppend < DESFIRE_EEPROM_BLOCK_SIZE)) {
        priorBlockBytesToCopy = 0;
    }
    uint16_t fileDataWriteAddr = SelectedFile.File.FileDataAddress + MIN(0, DESFIRE_BYTES_TO_BLOCKS(nextRecordIndexToAppend) - 1);
    uint8_t *writeDataBufStart = &Buffer[1];
    if (priorBlockBytesToCopy > 0) {
        uint8_t firstBlockData[DESFIRE_EEPROM_BLOCK_SIZE];
        ReadBlockBytes(firstBlockData, fileDataWriteAddr, priorBlockBytesToCopy);
        memcpy(firstBlockData + priorBlockBytesToCopy, writeDataBufStart, DESFIRE_EEPROM_BLOCK_SIZE - 1 - priorBlockBytesToCopy);
        WriteBlockBytes(firstBlockData, fileDataWriteAddr, DESFIRE_EEPROM_BLOCK_SIZE);
        fileDataWriteAddr += 1;
        appendRecordLength -= DESFIRE_EEPROM_BLOCK_SIZE - priorBlockBytesToCopy;
        writeDataBufStart += DESFIRE_EEPROM_BLOCK_SIZE - priorBlockBytesToCopy;
    }
    WriteBlockBytes(writeDataBufStart, fileDataWriteAddr, appendRecordLength);
    SelectedFile.File.RecordFile.RecordPointer = nextRecordPointer;
    WriteFileControlBlock(SelectedFile.Num, &(SelectedFile.File));
    Buffer[0] = ISO7816_CMD_NO_ERROR;
    Buffer[1] = ISO7816_CMD_NO_ERROR;
    return ISO7816_STATUS_RESPONSE_SIZE;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
