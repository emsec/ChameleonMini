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
#include "../Memory.h"
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

Iso7816WrappedCommandType_t Iso7816CmdType;

bool DesfireFromHalt = false;
BYTE DesfireCmdCLA = DESFIRE_NATIVE_CLA;

uint16_t ISO14443AStoreLastDataFrameAndReturn(const uint8_t *Buffer, uint16_t BufferBitCount) {
    if (BufferBitCount > 0) {
        uint16_t ISO14443ALastIncomingDataFrameBytes = MIN(ASBYTES(BufferBitCount), MAX_DATA_FRAME_XFER_SIZE);
        memcpy(&ISO14443ALastIncomingDataFrame[0], &Buffer[0], ISO14443ALastIncomingDataFrameBytes);
        ISO14443ALastIncomingDataFrameBits = MIN(BufferBitCount, ASBITS(MAX_DATA_FRAME_XFER_SIZE));
    }
    return ISO14443ALastIncomingDataFrameBits;
}

static void MifareDesfireAppInitLocal(uint8_t StorageSize, uint8_t Version, bool FormatPICC) {
    ResetLocalStructureData();
    DesfireState = DESFIRE_HALT;
    DesfireFromHalt = false;
    switch (Version) {
        case MIFARE_DESFIRE_EV2:
            InitialisePiccBackendEV1(StorageSize, FormatPICC);
            break;
        case MIFARE_DESFIRE_EV1:
            InitialisePiccBackendEV1(StorageSize, FormatPICC);
            break;
        case MIFARE_DESFIRE_EV0:
        default: /* Fall through: */
            InitialisePiccBackendEV0(StorageSize, FormatPICC);
            break;
    }
    if (FormatPICC) {
        MemoryStoreDesfireHeaderBytes();
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

void MifareDesfire4kEV2AppInit(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_4K, MIFARE_DESFIRE_EV2, false);
}

void MifareDesfire4kEV2AppInitRunOnce(void) {
    MifareDesfireAppInitLocal(DESFIRE_STORAGE_SIZE_4K, MIFARE_DESFIRE_EV2, true);
}

void MifareDesfireAppReset(void) {
    /* EMPTY: This is called repeatedly -- limit the amount of work done */
}

void MifareDesfireAppTick(void) {
    if (!CheckStateRetryCount(false)) {
        ResetISOState();
        MifareDesfireReset();
    }
}

void MifareDesfireAppTask(void) {
    /* EMPTY -- Do nothing. */
}

uint16_t MifareDesfireProcessCommand(uint8_t *Buffer, uint16_t ByteCount) {

    if (ByteCount == 0) {
        return ISO14443A_APP_NO_RESPONSE;
    } else if (Buffer[0] != STATUS_ADDITIONAL_FRAME) {
        DesfireState = DESFIRE_IDLE;
    }
    DesfireLogEntry(LOG_INFO_DESFIRE_INCOMING_DATA, (void *) Buffer, ByteCount);

    uint16_t ReturnBytes = 0;
    switch (DesfireState) {
        case DESFIRE_IDLE:
            ReturnBytes = CallInstructionHandler(Buffer, ByteCount);
            break;
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
        case DESFIRE_ISO7816_EXT_AUTH:
            ReturnBytes = ISO7816CmdExternalAuthenticate(Buffer, ByteCount);
            break;
        case DESFIRE_ISO7816_INT_AUTH:
            ReturnBytes = ISO7816CmdInternalAuthenticate(Buffer, ByteCount);
            break;
        case DESFIRE_ISO7816_GET_CHALLENGE:
            ReturnBytes = ISO7816CmdExternalAuthenticate(Buffer, ByteCount);
            break;
        case DESFIRE_READ_DATA_FILE:
            ReturnBytes = ReadDataFileIterator(Buffer);
            break;
        case DESFIRE_WRITE_DATA_FILE:
            ReturnBytes = WriteDataFileInternal(&Buffer[1], ByteCount - 1);
            break;
        default:
            Buffer[0] = STATUS_PICC_INTEGRITY_ERROR;
            ReturnBytes = DESFIRE_STATUS_RESPONSE_SIZE;
            break;
    }
    /* Reset the state retry count */
    StateRetryCount = 0;
    return ReturnBytes;

}

uint16_t MifareDesfireProcess(uint8_t *Buffer, uint16_t BitCount) {
    size_t ByteCount = ASBYTES(BitCount);
    if (ByteCount == 0) {
        DEBUG_PRINT_P(PSTR("RESEND LAST"));
        memcpy(&Buffer[0], &ISO14443ALastIncomingDataFrame[0], ASBYTES(ISO14443ALastIncomingDataFrameBits));
        return ISO14443ALastIncomingDataFrameBits;
    }
    DesfireCmdCLA = Buffer[0];
    if (ByteCount >= 2 && Buffer[1] == STATUS_ADDITIONAL_FRAME && DesfireCLA(Buffer[0])) {
        ByteCount -= 1;
        memmove(&Buffer[0], &Buffer[1], ByteCount);
        uint16_t ProcessedByteCount = MifareDesfireProcessCommand(Buffer, ByteCount);
        if (ProcessedByteCount != 0) {
            /* Re-wrap into padded APDU form */
            Buffer[ProcessedByteCount] = Buffer[0];
            memmove(&Buffer[0], &Buffer[1], ProcessedByteCount - 1);
            Buffer[ProcessedByteCount - 1] = 0x91;
            ++ProcessedByteCount;
        }
        return ASBITS(ProcessedByteCount);
    } else if ((ByteCount >= 5 && DesfireCLA(Buffer[0]) &&
                Buffer[2] == 0x00 && Buffer[3] == 0x00) || Iso7816CLA(DesfireCmdCLA)) {
        /* Wrapped native command structure or ISO7816: */
        if (Iso7816CLA(DesfireCmdCLA)) {
            uint16_t iso7816ParamsStatus = SetIso7816WrappedParametersType(Buffer, ByteCount);
            if (iso7816ParamsStatus != ISO7816_CMD_NO_ERROR) {
                Buffer[0] = (uint8_t)((iso7816ParamsStatus >> 8) & 0x00ff);
                Buffer[1] = (uint8_t)(iso7816ParamsStatus & 0x00ff);
                ByteCount = 2;
                return ASBITS(ByteCount);
            }
        }
        ByteCount = Buffer[4];
        Buffer[0] = Buffer[1];
        if (ByteCount > 0) {
            memmove(&Buffer[1], &Buffer[5], ByteCount);
        }
        /* Process the command */
        ByteCount = MifareDesfireProcessCommand(Buffer, ByteCount + 1);
        if (ByteCount != 0 && !Iso7816CLA(DesfireCmdCLA)) {
            /* Re-wrap into padded APDU form */
            Buffer[ByteCount] = Buffer[0];
            memmove(&Buffer[0], &Buffer[1], ByteCount - 1);
            Buffer[ByteCount - 1] = 0x91;
            ++ByteCount;
        } else {
            /* Re-wrap into ISO 7816-4 -- Done below by prepending the prologue back to the buffer */
        }
        return ASBITS(ByteCount);
    } else {
        /* ISO/IEC 14443-4 PDUs: No extra work */
        return ASBITS(MifareDesfireProcessCommand(Buffer, ByteCount));
    }

}

uint16_t MifareDesfireAppProcess(uint8_t *Buffer, uint16_t BitCount) {
    uint16_t ReturnBytes = 0;
    uint16_t ByteCount = MIN(ASBYTES(BitCount), MAX_DATA_FRAME_XFER_SIZE);
    if (ISO14443ALastIncomingDataFrameBits > 0 && BitCount == ISO14443ALastIncomingDataFrameBits &&
            !memcmp(&Buffer[0], &ISO14443ALastIncomingDataFrame[0], ByteCount)) {
        /* The PCD resent the same data frame (probably a synchronization issue):
         * Send out the same data as last time -- already the same as the Buffer contents:
         */
        return ISO14443ALastIncomingDataFrameBits;
    } else {
        ISO14443ALastIncomingDataFrameBits = ISO14443AStoreLastDataFrameAndReturn(Buffer, BitCount);
    }
    if (ByteCount >= 8 && DesfireCLA(Buffer[1]) && Buffer[2] == STATUS_ADDITIONAL_FRAME &&
            Buffer[3] == 0x00 && Buffer[4] == 0x00 && Buffer[5] == ByteCount - 8 &&
            DesfireStateExpectingAdditionalFrame(DesfireState)) {
        /* [PM3-V1] : Handle the native-wrapped version of the additional frame data: */
        uint16_t checkSumPostVerifyBytes = DesfirePreprocessAPDUAndTruncate(ActiveCommMode, Buffer, ByteCount);
        if (checkSumPostVerifyBytes == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        uint8_t PrologueCounterByte = Buffer[0];
        Buffer[0] = Buffer[2];
        ByteCount = Buffer[5];
        if (ByteCount > 0) {
            memmove(&Buffer[1], &Buffer[6], ByteCount);
        }
        uint16_t ProcessedByteCount = MifareDesfireProcessCommand(Buffer, ByteCount + 1);
        if (ProcessedByteCount == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Re-wrap into padded APDU form */
        Buffer[0] = PrologueCounterByte;
        uint16_t ProcessedByteCountWithCRCA = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount + 1);
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(ProcessedByteCountWithCRCA));
    } else if (ByteCount >= 3 && Buffer[2] == STATUS_ADDITIONAL_FRAME && DesfireStateExpectingAdditionalFrame(DesfireState)) {
        /* [PM3-V2] : Handle the ISO-prologue-only-wrapped version of the additional frame data: */
        uint8_t ISO7816PrologueBytes[2];
        memcpy(&ISO7816PrologueBytes[0], &Buffer[0], 2);
        uint16_t IncomingByteCount = DesfirePreprocessAPDUAndTruncate(ActiveCommMode, Buffer, ByteCount);
        if (IncomingByteCount == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        ByteCount = IncomingByteCount - 2;
        memmove(&Buffer[0], &Buffer[2], ByteCount);
        uint16_t ProcessedByteCount = MifareDesfireProcessCommand(Buffer, ByteCount);
        if (ProcessedByteCount > 0) {
            memmove(&Buffer[2], &Buffer[0], ProcessedByteCount);
        }
        memcpy(&Buffer[0], &ISO7816PrologueBytes[0], 2);
        ProcessedByteCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount + 2);
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(ProcessedByteCount));
    } else if (ByteCount >= 6 && DesfireCLA(Buffer[0]) && Buffer[1] == STATUS_ADDITIONAL_FRAME &&
               Buffer[2] == 0x00 && Buffer[3] == 0x00 && Buffer[4] == ByteCount - 8 &&
               DesfireStateExpectingAdditionalFrame(DesfireState)) {
        /* [PM3-V3] : Handle the native-wrapped version of the additional frame data: */
        uint16_t checkSumPostVerifyBytes = DesfirePreprocessAPDUAndTruncate(ActiveCommMode, Buffer, ByteCount);
        if (checkSumPostVerifyBytes == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        Buffer[0] = Buffer[1];
        ByteCount = Buffer[4];
        if (ByteCount > 0) {
            memmove(&Buffer[1], &Buffer[5], ByteCount);
        }
        uint16_t ProcessedByteCount = MifareDesfireProcessCommand(Buffer, ByteCount + 1);
        if (ProcessedByteCount == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Re-wrap into padded APDU form */
        Buffer[ProcessedByteCount] = Buffer[0];
        memmove(&Buffer[0], &Buffer[1], ProcessedByteCount - 1);
        Buffer[ProcessedByteCount - 1] = 0x91;
        ++ProcessedByteCount;
        ProcessedByteCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount + 2);
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(ProcessedByteCount));
    } else if (ByteCount >= 8 && DesfireCLA(Buffer[0]) &&
               Buffer[2] == 0x00 && Buffer[3] == 0x00 && Buffer[4] == ByteCount - 8) {
        DesfireCmdCLA = Buffer[0];
        uint16_t IncomingByteCount = ASBYTES(BitCount);
        uint16_t UnwrappedBitCount = ASBITS(DesfirePreprocessAPDU(ActiveCommMode, Buffer, IncomingByteCount));
        uint16_t ProcessedBitCount = MifareDesfireProcess(Buffer, UnwrappedBitCount);
        uint16_t ProcessedByteCount = ASBYTES(ProcessedBitCount);
        if (ProcessedByteCount == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        ProcessedByteCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount);
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(ProcessedByteCount));
    } else if ((ByteCount >= 8 && DesfireCLA(Buffer[1]) &&
                Buffer[3] == 0x00 && Buffer[4] == 0x00 && Buffer[5] == ByteCount - 8) ||
               (ByteCount >= 9 && DesfireCLA(Buffer[1]) &&
                Buffer[3] == 0x00 && Buffer[4] == 0x00 && Buffer[5] == ByteCount - 9)) {
        uint16_t UnwrappedByteCount = DesfirePreprocessAPDUAndTruncate(ActiveCommMode, Buffer, ByteCount);
        if (UnwrappedByteCount == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        uint8_t hf14AScanPrologue = Buffer[0];
        DesfireCmdCLA = Buffer[1];
        memmove(&Buffer[0], &Buffer[1], UnwrappedByteCount - 1);
        uint16_t UnwrappedBitCount = ASBITS(UnwrappedByteCount - 1);
        uint16_t ProcessedBitCount = MifareDesfireProcess(Buffer, UnwrappedBitCount);
        uint16_t ProcessedByteCount = ASBYTES(ProcessedBitCount);
        if (ProcessedByteCount++ == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        }
        memmove(&Buffer[1], &Buffer[0], ProcessedByteCount);
        Buffer[0] = hf14AScanPrologue;
        ProcessedByteCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount);
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(ProcessedByteCount));
    }
    Iso7816CmdType = IsWrappedISO7816CommandType(Buffer, ByteCount);
    if (Iso7816CmdType != ISO7816_WRAPPED_CMD_TYPE_NONE) {
        DesfireCmdCLA = (Iso7816CmdType == ISO7816_WRAPPED_CMD_TYPE_STANDARD) ? Buffer[2] : DESFIRE_NATIVE_CLA;
        uint8_t ISO7816PrologueBytes[2];
        memcpy(&ISO7816PrologueBytes[0], &Buffer[0], 2);
        ByteCount = DesfirePreprocessAPDU(ActiveCommMode, Buffer, ByteCount);
        if (ByteCount == 0) {
            return ISO14443A_APP_NO_RESPONSE;
        } else if (Iso7816CmdType == ISO7816_WRAPPED_CMD_TYPE_STANDARD) {
            memmove(&Buffer[0], &Buffer[2], ByteCount - 2);
            ByteCount = ByteCount - 2;
        } else if (Iso7816CmdType == ISO7816_WRAPPED_CMD_TYPE_PM3_ADDITIONAL_FRAME) {
            Buffer[0] = DesfireCmdCLA;
            Buffer[1] = STATUS_ADDITIONAL_FRAME;
            if (ByteCount > 3) {
                memmove(&Buffer[5], &Buffer[3], ByteCount - 3);
            }
            Buffer[2] = 0x00;
            Buffer[3] = 0x00;
            Buffer[4] = ByteCount - 5;
            ByteCount += 2;
        } else if (Iso7816CmdType == ISO7816_WRAPPED_CMD_TYPE_PM3RAW) {
            /* Something like the following (for PM3 raw ISO auth):
             * 0a 00 1a 00 CRC1 CRC2 -- first two are prologue -- last two are checksum
             */
            Buffer[0] = DesfireCmdCLA;
            Buffer[1] = Buffer[2];
            memmove(&Buffer[5], &Buffer[3], ByteCount - 3);
            Buffer[2] = 0x00;
            Buffer[3] = 0x00;
            Buffer[4] = ByteCount - 5;
        }
        uint16_t UnwrappedBitCount = ASBITS(ByteCount);
        uint16_t ProcessedBitCount = MifareDesfireProcess(Buffer, UnwrappedBitCount);
        uint16_t ProcessedByteCount = ASBYTES(ProcessedBitCount);
        /* Undo the leading 0x91(RCODE-XX) and shift for the PM3 raw wrapped commands: */
        if (Iso7816CmdType != ISO7816_WRAPPED_CMD_TYPE_STANDARD && ProcessedByteCount >= 1) {
            if (Buffer[ProcessedByteCount] != 0x00) {
                memmove(&Buffer[1], &Buffer[0], ProcessedByteCount);
                Buffer[0] = Buffer[ProcessedByteCount];
                --ProcessedByteCount;
            } else if (ProcessedByteCount >= 2) {
                ProcessedByteCount -= 2;
            }
        }
        /* Append the same ISO7816 prologue bytes to the response: */
        if (ProcessedByteCount > 0) {
            memmove(&Buffer[2], &Buffer[0], ProcessedByteCount);
        }
        memcpy(&Buffer[0], &ISO7816PrologueBytes[0], 2);
        ProcessedByteCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ProcessedByteCount + 2);
        return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(ProcessedByteCount));
    } else if (ByteCount >= 5 && (Iso144433AState == ISO14443_3A_STATE_IDLE || Iso144433AState == ISO14443_3A_STATE_ACTIVE) &&
               Buffer[0] != ISO14443A_CMD_RATS && Buffer[0] != ISO14443A_CMD_REQA &&
               !ISO14443ACmdIsWUPA(Buffer[0]) && !ISO144433AIsHalt(Buffer, BitCount)) {
        uint16_t IncomingByteCount = DesfirePreprocessAPDUAndTruncate(ActiveCommMode, Buffer, ByteCount);
        if (IncomingByteCount > 2) {
            IncomingByteCount -= 2;
            ReturnBytes = CallInstructionHandler(&Buffer[2], IncomingByteCount);
            if (ReturnBytes != ISO14443A_APP_NO_RESPONSE) {
                uint16_t OutgoingByteCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ReturnBytes + 2);
                return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(OutgoingByteCount));
            }
        }
    } else if (BitCount >= BITS_PER_BYTE && (Iso144433AState == ISO14443_3A_STATE_IDLE || Iso144433AState == ISO14443_3A_STATE_ACTIVE) &&
               Buffer[0] != ISO14443A_CMD_RATS && Buffer[0] != ISO14443A_CMD_REQA &&
               !ISO14443ACmdIsWUPA(Buffer[0]) && !ISO144433AIsHalt(Buffer, BitCount)) {
        /* This case should handle non-wrappped native commands. No pre/postprocessing afterwards: */
        ReturnBytes = CallInstructionHandler(Buffer, ByteCount);
        if (ReturnBytes != ISO14443A_APP_NO_RESPONSE) {
            uint16_t OutgoingByteCount = DesfirePostprocessAPDU(ActiveCommMode, Buffer, ReturnBytes);
            return ISO14443AStoreLastDataFrameAndReturn(Buffer, ASBITS(OutgoingByteCount));
        }
    }
    /* Default / Fall through -- No other cases matched -- Send it through the ISO14443A PICC handler: */
    uint16_t PiccProcessRespBits = ISO144433APiccProcess(Buffer, BitCount);
    uint16_t PiccProcessRespBytesCeil = ASBYTES(PiccProcessRespBits);
    if (PiccProcessRespBits >= BITS_PER_BYTE) {
        PiccProcessRespBits = ASBITS(PiccProcessRespBytesCeil);
    }
    return ISO14443AStoreLastDataFrameAndReturn(Buffer, PiccProcessRespBits);
}

void MifareDesfireReset(void) {
    InvalidateAuthState(false);
    DesfireState = DESFIRE_IDLE;
}

void ResetLocalStructureData(void) {
    memset(&Picc, PICC_FORMAT_BYTE, sizeof(Picc));
    memset(&AppDir, 0x00, sizeof(AppDir));
    memset(&SelectedApp, 0x00, sizeof(SelectedApp));
    memset(&SelectedFile, 0x00, sizeof(SelectedFile));
    memset(&TransferState, 0x00, sizeof(TransferState));
    SelectedApp.Slot = 0;
    SelectedFile.Num = -1;
    MifareDesfireReset();
    ResetISOState();
}

void ResetISOState(void) {
    ISO144433AReset();
    ISO144434Reset();
}


void MifareDesfireGetUid(ConfigurationUidType Uid) {
    GetPiccUid(Uid);
}

void MifareDesfireSetUid(ConfigurationUidType Uid) {
    SetPiccUid(Uid);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
