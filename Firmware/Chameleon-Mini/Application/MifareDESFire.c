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

#define MIFARE_DESFIRE_EV0     (0x00)
#define MIFARE_DESFIRE_EV1     (0x01)
#define MIFARE_DESFIRE_EV2     (0x02)

DesfireStateType DesfireState = DESFIRE_HALT;
DesfireStateType DesfirePreviousState = DESFIRE_IDLE;
bool DesfireFromHalt = false;
BYTE DesfireCmdCLA = DESFIRE_NATIVE_CLA;
static bool AnticolNoResp = false;

/* Dispatching routines */
void MifareDesfireReset(void) {
    AnticolNoResp = false;
}

static void MifareDesfireAppInitLocal(uint8_t StorageSize, uint8_t Version, bool FormatPICC) {
    ResetLocalStructureData();
    DesfireState = DESFIRE_IDLE;
    DesfireFromHalt = false;
    switch (Version) {
        case MIFARE_DESFIRE_EV1:
            InitialisePiccBackendEV1(StorageSize, FormatPICC);
            break;
        case MIFARE_DESFIRE_EV0:
        default: /* Fall through */
            InitialisePiccBackendEV0(StorageSize, FormatPICC);
            break;
    }
    DesfireCommMode = DESFIRE_DEFAULT_COMMS_STANDARD;
}

void MifareDesfireEV0AppInit(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_4K, MIFARE_DESFIRE_EV0, false);
}

void MifareDesfireEV0AppInitRunOnce(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_4K, MIFARE_DESFIRE_EV0, true);
}

void MifareDesfire2kEV1AppInit(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_2K, MIFARE_DESFIRE_EV1, false);
}

void MifareDesfire2kEV1AppInitRunOnce(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_2K, MIFARE_DESFIRE_EV1, true);
}

void MifareDesfire4kEV1AppInit(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_4K, MIFARE_DESFIRE_EV1, false);
}

void MifareDesfire4kEV1AppInitRunOnce(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_4K, MIFARE_DESFIRE_EV1, true);
}

void MifareDesfire8kEV1AppInit(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_8K, MIFARE_DESFIRE_EV1, false);
}

void MifareDesfire8kEV1AppInitRunOnce(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_8K, MIFARE_DESFIRE_EV1, true);
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
        return ISO14443A_APP_NO_RESPONSE;
    } else if ((DesfireCmdCLA != DESFIRE_NATIVE_CLA) &&
               (DesfireCmdCLA != DESFIRE_ISO7816_CLA)) {
        return ISO14443A_APP_NO_RESPONSE;
    } else if (Buffer[0] != STATUS_ADDITIONAL_FRAME) {
        DesfireState = DESFIRE_IDLE;
        uint16_t ReturnBytes = CallInstructionHandler(Buffer, ByteCount);
        return ReturnBytes;
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
    //LogEntry(LOG_INFO_DESFIRE_INCOMING_DATA, Buffer, ByteCount);
    if (BitCount == 0) {
        LogEntry(LOG_INFO_DESFIRE_INCOMING_DATA, Buffer, ByteCount);
        return ISO14443A_APP_NO_RESPONSE;
    } else if ((ByteCount >= 8 && DesfireCLA(Buffer[0]) && Buffer[2] == 0x00 &&
                Buffer[3] == 0x00 && (Buffer[4] == ByteCount - 6 || Buffer[4] == ByteCount - 8)) || Iso7816CLA(DesfireCmdCLA)) {
        /* Wrapped native command structure or ISO7816: */
        if (Iso7816CLA(DesfireCmdCLA)) {
            uint16_t iso7816ParamsStatus = SetIso7816WrappedParametersType(Buffer, ByteCount);
            if (iso7816ParamsStatus != ISO7816_CMD_NO_ERROR) {
                Buffer[0] = (uint8_t)((iso7816ParamsStatus >> 8) & 0x00ff);
                Buffer[1] = (uint8_t)(iso7816ParamsStatus & 0x00ff);
                ByteCount = 2;
                return ByteCount * BITS_PER_BYTE;
            }
        }
        ByteCount = Buffer[4];
        Buffer[0] = Buffer[1];
        memmove(&Buffer[1], &Buffer[5], ByteCount);
        /* Process the command */
        ByteCount = MifareDesfireProcessCommand(Buffer, ByteCount + 1);
        if ((ByteCount != 0 && !Iso7816CLA(DesfireCmdCLA)) || (ByteCount == 1)) {
            /* Re-wrap into padded APDU form */
            Buffer[ByteCount] = Buffer[0];
            memmove(&Buffer[0], &Buffer[1], ByteCount - 1);
            Buffer[ByteCount - 1] = 0x91;
            ++ByteCount;
        } else {
            /* Re-wrap into ISO 7816-4 */
        }
        //LogEntry(LOG_INFO_DESFIRE_OUTGOING_DATA, Buffer, ByteCount);
        return ByteCount * BITS_PER_BYTE;
    } else {
        /* ISO/IEC 14443-4 PDUs: No extra work */
        return MifareDesfireProcessCommand(Buffer, ByteCount) * BITS_PER_BYTE;
    }

}

uint16_t MifareDesfireAppProcess(uint8_t *Buffer, uint16_t BitCount) {
    uint16_t ByteCount = (BitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    uint16_t ReturnedBytes = 0;
    if (ByteCount >= 8 && DesfireCLA(Buffer[0]) && Buffer[2] == 0x00 &&
            Buffer[3] == 0x00 && Buffer[4] == ByteCount - 8) {
        DesfireCmdCLA = Buffer[0];
        uint16_t IncomingByteCount = (BitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
        uint16_t UnwrappedBitCount = DesfirePreprocessAPDU(ActiveCommMode, Buffer, IncomingByteCount) * BITS_PER_BYTE;
        uint16_t ProcessedBitCount = MifareDesfireProcess(Buffer, UnwrappedBitCount);
        uint16_t ProcessedByteCount = (ProcessedBitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount) * BITS_PER_BYTE);
    } else if (IsWrappedISO7816CommandType(Buffer, ByteCount)) {
        DesfireCmdCLA = Buffer[2];
        uint8_t ISO7816PrologueBytes[2];
        memcpy(&ISO7816PrologueBytes[0], Buffer, 2);
        uint16_t IncomingByteCount = DesfirePreprocessAPDU(ActiveCommMode, Buffer, IncomingByteCount);
        memmove(&Buffer[0], &Buffer[2], IncomingByteCount - 2);
        uint16_t UnwrappedBitCount = (IncomingByteCount - 2) * BITS_PER_BYTE;
        uint16_t ProcessedBitCount = MifareDesfireProcess(Buffer, UnwrappedBitCount);
        uint16_t ProcessedByteCount = (ProcessedBitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
        /* Append the same ISO7816 prologue bytes to the response: */
        memmove(&Buffer[2], &Buffer[0], ProcessedByteCount);
        memcpy(&Buffer[0], &ISO7816PrologueBytes[0], 2);
        ProcessedBitCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount + 2) * BITS_PER_BYTE;
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ProcessedBitCount);
    } else if ((ReturnedBytes = CallInstructionHandler(Buffer, ByteCount)) != ISO14443A_APP_NO_RESPONSE) {
        /* This case should handle non-wrappped native commands. No pre/postprocessing afterwards: */
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ReturnedBytes * BITS_PER_BYTE);
    } else if (!AnticolNoResp || BitCount < BITS_PER_BYTE) {
        /* This case is to exchange anticollision loop and RATS data. No need to pre/postprocess it depending
         * on the CommMode, which has not been set yet if we reach this point:
         */
        uint16_t PiccProcessRespBits = ISO144433APiccProcess(Buffer, BitCount);
        if (PiccProcessRespBits == ISO14443A_APP_NO_RESPONSE) {
            // Stop pesky USB readers trying to autodetect all tag types by brute-force enumeration
            // from interfering with making it into the command exchange (DESFIRE_IDLE) states.
            // Once the anticollision and/or RATS has completed, set this flag to keep it from
            // resending that initial handshaking until the AppReset() function is called on a timeout.
            // N.b., the ACR-122 reader does this repeatedly when trying to run the LibNFC testing code
            // even when the reader has not been put in scan mode --
            // and it really screws things up timing-wise!
            AnticolNoResp = true;
        }
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, PiccProcessRespBits);
    }
    LogEntry(LOG_INFO_DESFIRE_OUTGOING_DATA, Buffer, ByteCount);
    return ISO14443A_APP_NO_RESPONSE;
}

void ResetLocalStructureData(void) {
    AnticolNoResp = false;
    DesfirePreviousState = DESFIRE_IDLE;
    DesfireState = DESFIRE_HALT;
    InvalidateAuthState(0x00);
    memset(&Picc, PICC_FORMAT_BYTE, sizeof(Picc));
    memset(&AppDir, 0x00, sizeof(AppDir));
    memset(&SelectedApp, 0x00, sizeof(SelectedApp));
    memset(&SelectedFile, 0x00, sizeof(SelectedFile));
    memset(&TransferState, 0x00, sizeof(TransferState));
    memset(&SessionKey, 0x00, sizeof(CryptoKeyBufferType));
    memset(&SessionIV, 0x00, sizeof(CryptoIVBufferType));
    SessionIVByteSize = 0x00;
    SelectedApp.Slot = 0;
    SelectedFile.Num = -1;
}

void MifareDesfireGetUid(ConfigurationUidType Uid) {
    GetPiccUid(Uid);
}

void MifareDesfireSetUid(ConfigurationUidType Uid) {
    SetPiccUid(Uid);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
