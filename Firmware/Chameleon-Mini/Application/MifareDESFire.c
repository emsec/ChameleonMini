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
 * MifareDesfire.c
 * MIFARE DESFire frontend
 *
 * Created on: 14.10.2016
 * Author: dev_zzo
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../Common.h"
#include "Reader14443A.h"

#include "MifareDESFire.h"
#include "DESFire/DESFireFirmwareSettings.h"
#include "DESFire/DESFireInstructions.h"
#include "DESFire/DESFirePICCControl.h"
#include "DESFire/DESFireCrypto.h"
#include "DESFire/DESFireISO14443Support.h"
#include "DESFire/DESFireISO7816Support.h"
#include "DESFire/DESFireStatusCodes.h"
#include "DESFire/DESFireLogging.h"
#include "DESFire/DESFireUtils.h"

DesfireStateType DesfireState = DESFIRE_HALT;
DesfireStateType DesfirePreviousState = DESFIRE_IDLE;
bool DesfireFromHalt = false;
BYTE DesfireCmdCLA = DESFIRE_NATIVE_CLA;

/* Dispatching routines */
void MifareDesfireReset(void) {}

void MifareDesfireEV0AppInit(void) {
    /* Init lower layers: nothing for now */
    ResetLocalStructureData();
    DesfireState = DESFIRE_IDLE;
    DesfireFromHalt = false;
    InitialisePiccBackendEV0(DESFIRE_STORAGE_SIZE_4K);
    /* The rest is handled in reset */
}

static void MifareDesfireEV1AppInit(uint8_t StorageSize) {
    /* Init lower layers: nothing for now */
    ResetLocalStructureData();
    DesfireState = DESFIRE_IDLE;
    DesfireFromHalt = false;
    InitialisePiccBackendEV1(StorageSize);
    /* The rest is handled in reset */
}

void MifareDesfire2kEV1AppInit(void) {
    ResetLocalStructureData();
    DesfireState = DESFIRE_IDLE;
    DesfireFromHalt = false;
    MifareDesfireEV1AppInit(DESFIRE_STORAGE_SIZE_2K);
}

void MifareDesfire4kEV1AppInit(void) {
    ResetLocalStructureData();
    DesfireState = DESFIRE_IDLE;
    DesfireFromHalt = false;
    MifareDesfireEV1AppInit(DESFIRE_STORAGE_SIZE_4K);
}

void MifareDesfire8kEV1AppInit(void) {
    ResetLocalStructureData();
    DesfireState = DESFIRE_IDLE;
    DesfireFromHalt = false;
    MifareDesfireEV1AppInit(DESFIRE_STORAGE_SIZE_8K);
}

void MifareDesfireAppReset(void) {
    /* This is called repeatedly, so limit the amount of work done */
    ISO144433AReset();
    ISO144434Reset();
    MifareDesfireReset();
}

void MifareDesfireAppTick(void) {
    if (!CheckStateRetryCount2(false, false)) {
        MifareDesfireAppReset();
    }
    /* Empty */
}

void MifareDesfireAppTask(void) {
    /* Empty */
}

uint16_t MifareDesfireProcessCommand(uint8_t *Buffer, uint16_t ByteCount) {
    if (ByteCount == 0) {
        return ISO14443A_APP_NO_RESPONSE;//
    } else if ((DesfireCmdCLA != DESFIRE_NATIVE_CLA) &&
               (DesfireCmdCLA != DESFIRE_ISO7816_CLA)) {
        return ISO14443A_APP_NO_RESPONSE;
    } else if (Buffer[0] != STATUS_ADDITIONAL_FRAME) {
        uint16_t ReturnBytes = CallInstructionHandler(Buffer, ByteCount);
        return ReturnBytes;
    }

    /* Expecting further data here */
    if (Buffer[0] != STATUS_ADDITIONAL_FRAME) {
        return ISO14443A_APP_NO_RESPONSE;
    }

    uint16_t ReturnBytes = 0;
    switch (DesfireState) {
        case DESFIRE_GET_VERSION2:
            ReturnBytes = EV0CmdGetVersion2(Buffer, ByteCount);
            break;
        case DESFIRE_GET_VERSION3:
            ReturnBytes = EV0CmdGetVersion3(Buffer, ByteCount);
            break;
        case DESFIRE_GET_APPLICATION_IDS2:
            ReturnBytes = GetApplicationIdsIterator(Buffer, ByteCount);
            break;
        case DESFIRE_LEGACY_AUTHENTICATE2:
            ReturnBytes = EV0CmdAuthenticateLegacy2(Buffer, ByteCount);
            break;
        case DESFIRE_ISO_AUTHENTICATE2:
            ReturnBytes = DesfireCmdAuthenticate3KTDEA2(Buffer, ByteCount);
            break;
        case DESFIRE_AES_AUTHENTICATE2:
            ReturnBytes = DesfireCmdAuthenticateAES2(Buffer, ByteCount);
            break;
        case DESFIRE_READ_DATA_FILE:
            ReturnBytes = ReadDataFileIterator(Buffer);
            break;
        case DESFIRE_WRITE_DATA_FILE:
            ReturnBytes = WriteDataFileInternal(&Buffer[1], ByteCount - 1);
            break;
        default:
            /* Should not happen. */
            Buffer[0] = STATUS_PICC_INTEGRITY_ERROR;
            return DESFIRE_STATUS_RESPONSE_SIZE;
    }
    return ReturnBytes;

}

uint16_t MifareDesfireProcess(uint8_t *Buffer, uint16_t BitCount) {
    size_t ByteCount = (BitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    DesfireCmdCLA = Buffer[0];
    if ((ByteCount >= 8 && DesfireCLA(Buffer[0]) && Buffer[2] == 0x00 &&
            Buffer[3] == 0x00 && Buffer[4] == ByteCount - 8) || Iso7816CLA(DesfireCmdCLA)) { // Wrapped native command structure:
        /* Unwrap the PDU from ISO 7816-4 */
        // Check CRC bytes appended to the buffer:
        // -- Actually, just ignore parity problems if they exist,
        if (Iso7816CLA(DesfireCmdCLA)) {
            uint16_t iso7816ParamsStatus = SetIso7816WrappedParametersType(Buffer, ByteCount);
            if (iso7816ParamsStatus != ISO7816_CMD_NO_ERROR) {
                Buffer[0] = (uint8_t)((iso7816ParamsStatus >> 8) & 0x00ff);
                Buffer[1] = (uint8_t)(iso7816ParamsStatus & 0x00ff);
                ISO14443AAppendCRCA(Buffer, 2);
                ByteCount = 2 + 2;
                return ByteCount * BITS_PER_BYTE;
            }
        }
        ByteCount = Buffer[4]; // also removing the trailing two parity bytes
        Buffer[0] = Buffer[1];
        memmove(&Buffer[1], &Buffer[5], ByteCount);
        /* Process the command */
        /* TODO: Where are we deciphering wrapped payload data?
         *       This should depend on the CommMode standard?
         */
        ByteCount = MifareDesfireProcessCommand(Buffer, ByteCount + 1);
        /* TODO: Where are we re-wrapping the data according to the CommMode standards? */
        if ((ByteCount != 0 && !Iso7816CLA(DesfireCmdCLA)) || (ByteCount == 1)) {
            /* Re-wrap into padded APDU form */
            Buffer[ByteCount] = Buffer[0];
            memmove(&Buffer[0], &Buffer[1], ByteCount - 1);
            Buffer[ByteCount - 1] = 0x91;
            ISO14443AAppendCRCA(Buffer, ++ByteCount);
            ByteCount += 2;
        } else {
            /* Re-wrap into ISO 7816-4 */
            //Buffer[ByteCount] = Buffer[0];
            //Buffer[ByteCount + 1] = Buffer[1];
            //memmove(&Buffer[0], &Buffer[2], ByteCount - 2);
            ISO14443AAppendCRCA(Buffer, ByteCount);
            ByteCount += 2;
        }
        //LogEntry(LOG_INFO_DESFIRE_OUTGOING_DATA, Buffer, ByteCount);
        return ByteCount * BITS_PER_BYTE;
    } else {
        /* ISO/IEC 14443-4 PDUs: No extra work */
        return MifareDesfireProcessCommand(Buffer, ByteCount) * BITS_PER_BYTE;
    }

}

uint16_t MifareDesfireAppProcess(uint8_t *Buffer, uint16_t BitCount) {
    size_t ByteCount = (BitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    if (ByteCount >= 8 && DesfireCLA(Buffer[0]) && Buffer[2] == 0x00 &&
            Buffer[3] == 0x00 && Buffer[4] == ByteCount - 8) {
        return MifareDesfireProcess(Buffer, BitCount);
    } else if (ByteCount >= 6 && DesfireCLA(Buffer[0]) && Buffer[2] == 0x00 &&
               Buffer[3] == 0x00 && Buffer[4] == ByteCount - 6) {
        // Native wrapped command send without CRCA checksum bytes appended:
        return MifareDesfireProcess(Buffer, BitCount);
    } else if (IsWrappedISO7816CommandType(Buffer, ByteCount)) {
        uint8_t ISO7816PrologueBytes[2];
        memcpy(&ISO7816PrologueBytes[0], Buffer, 2);
        memmove(&Buffer[0], &Buffer[2], ByteCount - 2);
        uint16_t ProcessedBitCount = MifareDesfireProcess(Buffer, BitCount);
        uint16_t ProcessedByteCount = (ProcessedBitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
        /* Append the same ISO7816 prologue bytes to the response: */
        memmove(&Buffer[2], &Buffer[0], ProcessedByteCount);
        memcpy(&Buffer[0], &ISO7816PrologueBytes[0], 2);
        ISO14443AAppendCRCA(Buffer, ProcessedByteCount);
        ProcessedBitCount += 2 * BITS_PER_BYTE;
        return ProcessedBitCount;
    } else {
        uint16_t ProcessedBitCount = ISO144433APiccProcess(Buffer, BitCount);
        if (ProcessedBitCount == ISO14443A_APP_NO_RESPONSE) {
            const char *debugNoResponse = PSTR("APP_NO_RESP");
            LogDebuggingMsg(debugNoResponse);
        }
        return ProcessedBitCount;
    }
}

void ResetLocalStructureData(void) {
    DesfirePreviousState = DESFIRE_IDLE;
    DesfireState = DESFIRE_HALT;
    InvalidateAuthState(0x00);
    memset(&Picc, PICC_FORMAT_BYTE, sizeof(Picc));
    memset(&SelectedApp, 0x00, sizeof(SelectedApp));
    memset(&SelectedFile, 0x00, sizeof(SelectedFile));
    memset(&TransferState, 0x00, sizeof(TransferState));
    memset(&SessionKey, 0x00, sizeof(CryptoKeyBufferType));
    memset(&SessionIV, 0x00, sizeof(CryptoIVBufferType));
    SessionIVByteSize = 0x00;
    memset(&AESCryptoSessionKey, 0x00, sizeof(DesfireAESCryptoKey));
    memset(&AESCryptoIVBuffer, 0x00, sizeof(DesfireAESCryptoKey));
}

void MifareDesfireGetUid(ConfigurationUidType Uid) {
    GetPiccUid(Uid);
}

void MifareDesfireSetUid(ConfigurationUidType Uid) {
    SetPiccUid(Uid);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
