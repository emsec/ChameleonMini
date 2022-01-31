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
 * DESFireCrypto.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include <string.h>

#include "../../Log.h"

#include "../MifareDESFire.h"
#include "DESFireCrypto.h"
#include "DESFireInstructions.h"
#include "DESFirePICCControl.h"
#include "DESFireISO14443Support.h"
#include "DESFireStatusCodes.h"
#include "DESFireLogging.h"

CryptoKeyBufferType SessionKey = { 0 };
CryptoIVBufferType SessionIV = { 0 };
BYTE SessionIVByteSize = { 0 };

uint16_t AESCryptoKeySizeBytes = 0;
CryptoAESConfig_t AESCryptoContext = { 0 };
DesfireAESCryptoKey AESCryptoSessionKey = { 0 };
DesfireAESCryptoKey AESCryptoIVBuffer = { 0 };

uint8_t Authenticated = 0x00;
uint8_t AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
uint8_t AuthenticatedWithPICCMasterKey = 0x00;
uint8_t CryptoAuthMethod = CRYPTO_TYPE_ANY;
uint8_t ActiveCommMode = DESFIRE_DEFAULT_COMMS_STANDARD;

void InvalidateAuthState(BYTE keepPICCAuthData) {
    if (!keepPICCAuthData) {
        AuthenticatedWithPICCMasterKey = DESFIRE_NOT_AUTHENTICATED;
    }
    Authenticated = 0x00;
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    AuthenticatedWithPICCMasterKey = 0x00;
    Iso7816FileSelected = false;
    CryptoAuthMethod = CRYPTO_TYPE_ANY;
    ActiveCommMode = DESFIRE_DEFAULT_COMMS_STANDARD;
}

bool IsAuthenticated(void) {
    return Authenticated != 0x00;
}

BYTE GetDefaultCryptoMethodKeySize(uint8_t cryptoType) {
    switch (cryptoType) {
        case CRYPTO_TYPE_2KTDEA:
            return CRYPTO_2KTDEA_KEY_SIZE;
        case CRYPTO_TYPE_3K3DES:
            return CRYPTO_3KTDEA_KEY_SIZE;
        case CRYPTO_TYPE_AES128:
            return 16;
        default:
            return 8;
    }
}

const char *GetCryptoMethodDesc(uint8_t cryptoType) {
    switch (cryptoType) {
        case CRYPTO_TYPE_2KTDEA:
            return PSTR("2KTDEA");
        case CRYPTO_TYPE_3K3DES:
            return PSTR("3K3DES");
        case CRYPTO_TYPE_AES128:
            return PSTR("AES128");
        default:
            return PSTR("ANY");
    }
}

BYTE GetCryptoMethodCommSettings(uint8_t cryptoType) {
    switch (cryptoType) {
        case CRYPTO_TYPE_2KTDEA:
            return DESFIRE_COMMS_PLAINTEXT_MAC;
        case CRYPTO_TYPE_3K3DES:
            return DESFIRE_COMMS_CIPHERTEXT_DES;
        case CRYPTO_TYPE_AES128:
            return DESFIRE_COMMS_CIPHERTEXT_AES128;
        default:
            return DESFIRE_COMMS_PLAINTEXT;
    }
}

const char *GetCommSettingsDesc(uint8_t cryptoType) {
    switch (cryptoType) {
        case CRYPTO_TYPE_2KTDEA:
            return PSTR("PTEXT-MAC");
        case CRYPTO_TYPE_3K3DES:
            return PSTR("CTEXT-DES");
        case CRYPTO_TYPE_AES128:
            return PSTR("CTEXT-AES128-CMAC");
        default:
            return PSTR("PTEXT-DEFAULT");
    }
}


BYTE GetCryptoKeyTypeFromAuthenticateMethod(BYTE authCmdMethod) {
    switch (authCmdMethod) {
        case CMD_AUTHENTICATE_AES:
        case CMD_AUTHENTICATE_EV2_FIRST:
        case CMD_AUTHENTICATE_EV2_NONFIRST:
            return CRYPTO_TYPE_AES128;
        case CMD_AUTHENTICATE_ISO:
            return CRYPTO_TYPE_DES;
        case CMD_AUTHENTICATE:
        case CMD_ISO7816_EXTERNAL_AUTHENTICATE:
        case CMD_ISO7816_INTERNAL_AUTHENTICATE:
        default:
            return CRYPTO_TYPE_2KTDEA;
    }
}

void InitAESCryptoKeyData(DesfireAESCryptoKey *cryptoKeyData) {
    memset(cryptoKeyData, 0x00, sizeof(DesfireAESCryptoKey));
}
uint8_t CryptoAESTransferEncryptSend(uint8_t *Buffer, uint8_t Count, const uint8_t *Key) {
    uint8_t AvailablePlaintext = TransferState.ReadData.Encryption.AvailablePlaintext;
    uint8_t TempBuffer[(DESFIRE_MAX_PAYLOAD_AES_BLOCKS + 1) * CRYPTO_DES_BLOCK_SIZE];
    uint16_t tempBufSize = (DESFIRE_MAX_PAYLOAD_AES_BLOCKS + 1) * CRYPTO_DES_BLOCK_SIZE;
    uint16_t bufFillSize = MIN(tempBufSize, AvailablePlaintext), bufFillSize2;
    uint8_t *tempBufOffset;
    if (AvailablePlaintext) {
        /* Fill the partial block */
        memcpy(&TempBuffer[0], &TransferState.BlockBuffer[0], bufFillSize);
    }
    /* Copy fresh plaintext to the temp buffer */
    if (Count > bufFillSize && tempBufSize - bufFillSize > 0) {
        tempBufOffset = &TempBuffer[bufFillSize];
        bufFillSize2 = bufFillSize;
        bufFillSize = MIN(Count, tempBufSize - bufFillSize);
        memcpy(tempBufOffset, Buffer, bufFillSize);
        Count += bufFillSize2 + Count - bufFillSize;
    }
    uint8_t BlockCount = Count / CRYPTO_AES_BLOCK_SIZE;
    /* Stash extra plaintext for later */
    AvailablePlaintext = Count - BlockCount * CRYPTO_AES_BLOCK_SIZE;
    if (AvailablePlaintext) {
        memcpy(&TransferState.BlockBuffer[0],
               &Buffer[BlockCount * CRYPTO_AES_BLOCK_SIZE], AvailablePlaintext);
    }
    TransferState.ReadData.Encryption.AvailablePlaintext = AvailablePlaintext;
    /* Encrypt complete blocks in the buffer */
    uint8_t zeroIV[CRYPTO_AES_BLOCK_SIZE];
    memset(zeroIV, 0x00, CRYPTO_AES_BLOCK_SIZE);
    CryptoAESEncrypt_CBCSend(BlockCount, &TempBuffer[0], &Buffer[0],
                             *Key, zeroIV);
    /* Return byte count to transfer */
    return BlockCount * CRYPTO_AES_BLOCK_SIZE;
}

uint8_t CryptoAESTransferEncryptReceive(uint8_t *Buffer, uint8_t Count, const uint8_t *Key) {
    LogEntry(LOG_INFO_DESFIRE_INCOMING_DATA_ENC, Buffer, Count);
    return STATUS_OPERATION_OK;
}

/* Checksum routines */

void TransferChecksumUpdateCRCA(const uint8_t *Buffer, uint8_t Count) {
    TransferState.Checksums.MACData.CRCA =
        ISO14443AUpdateCRCA(Buffer, Count, TransferState.Checksums.MACData.CRCA);
}

uint8_t TransferChecksumFinalCRCA(uint8_t *Buffer) {
    /* Copy the checksum to destination */
    memcpy(Buffer, &TransferState.Checksums.MACData.CRCA, 2);
    /* Return the checksum size */
    return 2;
}

void TransferChecksumUpdateMACTDEA(const uint8_t *Buffer, uint8_t Count) {
    uint8_t AvailablePlaintext = TransferState.Checksums.AvailablePlaintext;
    uint8_t TempBuffer[CRYPTO_DES_BLOCK_SIZE];

    if (AvailablePlaintext) {
        uint8_t TempBytes;
        /* Fill the partial block */
        TempBytes = CRYPTO_DES_BLOCK_SIZE - AvailablePlaintext;
        if (TempBytes > Count)
            TempBytes = Count;
        memcpy(&TransferState.BlockBuffer[AvailablePlaintext], &Buffer[0], TempBytes);
        Count -= TempBytes;
        Buffer += TempBytes;
        /* MAC the partial block */
        TransferState.Checksums.MACData.CryptoChecksumFunc.TDEAFunc(1, &TransferState.BlockBuffer[0],
                                                                    &TempBuffer[0], SessionIV, SessionKey);
    }
    /* MAC complete blocks in the buffer */
    while (Count >= CRYPTO_DES_BLOCK_SIZE) {
        /* NOTE: This is block-by-block, hence slow.
         *       See if it's better to just allocate a temp buffer large enough (64 bytes). */
        TransferState.Checksums.MACData.CryptoChecksumFunc.TDEAFunc(1, &Buffer[0], &TempBuffer[0],
                                                                    SessionIV, SessionKey);
        Count -= CRYPTO_DES_BLOCK_SIZE;
        Buffer += CRYPTO_DES_BLOCK_SIZE;
    }
    /* Copy the new partial block */
    if (Count) {
        memcpy(&TransferState.BlockBuffer[0], &Buffer[0], Count);
    }
    TransferState.Checksums.AvailablePlaintext = Count;
}

uint8_t TransferChecksumFinalMACTDEA(uint8_t *Buffer) {
    uint8_t AvailablePlaintext = TransferState.Checksums.AvailablePlaintext;
    uint8_t TempBuffer[CRYPTO_DES_BLOCK_SIZE];

    if (AvailablePlaintext) {
        /* Apply padding */
        CryptoPaddingTDEA(&TransferState.BlockBuffer[0], AvailablePlaintext, false);
        /* MAC the partial block */
        TransferState.Checksums.MACData.CryptoChecksumFunc.TDEAFunc(1, &TransferState.BlockBuffer[0],
                                                                    &TempBuffer[0], SessionIV, SessionKey);
        TransferState.Checksums.AvailablePlaintext = 0;
    }
    /* Copy the checksum to destination */
    memcpy(Buffer, SessionIV, 4);
    /* Return the checksum size */
    return 4;
}

/* Encryption routines */

uint8_t TransferEncryptTDEASend(uint8_t *Buffer, uint8_t Count) {
    uint8_t AvailablePlaintext = TransferState.ReadData.Encryption.AvailablePlaintext;
    uint8_t TempBuffer[(DESFIRE_MAX_PAYLOAD_TDEA_BLOCKS + 1) * CRYPTO_DES_BLOCK_SIZE];
    uint8_t BlockCount;

    if (AvailablePlaintext) {
        /* Fill the partial block */
        memcpy(&TempBuffer[0], &TransferState.BlockBuffer[0], AvailablePlaintext);
    }
    /* Copy fresh plaintext to the temp buffer */
    memcpy(&TempBuffer[AvailablePlaintext], Buffer, Count);
    Count += AvailablePlaintext;
    BlockCount = Count / CRYPTO_DES_BLOCK_SIZE;
    /* Stash extra plaintext for later */
    AvailablePlaintext = Count - BlockCount * CRYPTO_DES_BLOCK_SIZE;
    if (AvailablePlaintext) {
        memcpy(&TransferState.BlockBuffer[0],
               &Buffer[BlockCount * CRYPTO_DES_BLOCK_SIZE], AvailablePlaintext);
    }
    TransferState.ReadData.Encryption.AvailablePlaintext = AvailablePlaintext;
    /* Encrypt complete blocks in the buffer */
    CryptoEncrypt2KTDEA_CBCSend(BlockCount, &TempBuffer[0], &Buffer[0],
                                SessionIV, SessionKey);
    /* Return byte count to transfer */
    return BlockCount * CRYPTO_DES_BLOCK_SIZE;
}

uint8_t TransferEncryptTDEAReceive(uint8_t *Buffer, uint8_t Count) {
    LogEntry(LOG_INFO_DESFIRE_INCOMING_DATA_ENC, Buffer, Count);
    return 0;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
