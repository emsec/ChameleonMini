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
 * CryptoCMAC.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#include <string.h>

#include "CryptoCMAC.h"
#include "DESFire/DESFireUtils.h"
#include "DESFire/DESFireCrypto.h"
#include "CryptoTDEA.h"
#include "CryptoAES128.h"

static uint8_t _cmac_K1[CRYPTO_MAX_BLOCK_SIZE] = { 0x00 };
static uint8_t _cmac_K2[CRYPTO_MAX_BLOCK_SIZE] = { 0x00 };
static uint8_t _cmac_RB[CRYPTO_MAX_BLOCK_SIZE] = { 0x00 };
static uint8_t _cmac_final[CRYPTO_MAX_BLOCK_SIZE] = { 0x00 };
static uint8_t _cmac_zeros[CRYPTO_MAX_BLOCK_SIZE] = { 0x00 };
static uint8_t _mac_key24[CRYPTO_MAX_KEY_SIZE] = { 0x00 };

static void getCMACSubK1(const uint8_t *bufferL, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut);
static void getCMACSubK1(const uint8_t *bufferL, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut) {
    _cmac_RB[blockSize - 1] = polyByte;
    RotateArrayLeft(bufferL, bufferOut, blockSize);
    if ((bufferL[0] & 0x80) != 0) {
        for (int kidx = 0; kidx < blockSize; kidx++) {
            bufferOut[kidx] = (uint8_t)(bufferOut[kidx] ^ _cmac_RB[kidx]);
        }
    }
}

static void getCMACSubK2(const uint8_t *bufferK1, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut);
static void getCMACSubK2(const uint8_t *bufferK1, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut) {
    _cmac_RB[blockSize - 1] = polyByte;
    RotateArrayLeft(bufferK1, bufferOut, blockSize);
    if ((bufferK1[0] & 0x80) != 0) {
        for (int kidx = 0; kidx < blockSize; kidx++) {
            bufferOut[kidx] = (uint8_t)(bufferOut[kidx] ^ _cmac_RB[kidx]);
        }
    }
}

static bool appendBufferCMACSubroutine(uint8_t cryptoType, const uint8_t *keyData, uint8_t *bufferK1, uint8_t *bufferK2, uint8_t *bufferIV, uint8_t blockSize, uint8_t *bufferOut, uint16_t bufferSize);
static bool appendBufferCMACSubroutine(uint8_t cryptoType, const uint8_t *keyData, uint8_t *bufferK1, uint8_t *bufferK2, uint8_t *bufferIV, uint8_t blockSize, uint8_t *bufferOut, uint16_t bufferSize) {
    uint16_t newBlockSize = bufferSize;
    if (bufferSize == 0) {
        memset(bufferOut, 0x00, blockSize);
        bufferOut[0] = (uint8_t) 0x80;
        newBlockSize = blockSize;
    } else if ((bufferSize % blockSize) != 0) {
        newBlockSize = bufferSize - (bufferSize % blockSize) + blockSize;
        bufferOut[bufferSize] = (uint8_t) 0x80;
    }
    if (bufferSize != 0 && (bufferSize % blockSize) == 0) {
        // Complete block (use K1):
        for (int i = bufferSize - blockSize; i < bufferSize; i++) {
            bufferOut[i] ^= bufferK1[i - bufferSize + blockSize];
        }
    } else {
        // Incomplete block (use K2):
        for (int i = bufferSize - blockSize; i < bufferSize; i++) {
            bufferOut[i] ^= bufferK2[i - bufferSize + blockSize];
        }
    }
    switch (cryptoType) {
        case CRYPTO_TYPE_3K3DES:
            Encrypt3DESBuffer(newBlockSize, bufferOut, &bufferOut[bufferSize], keyData, bufferIV);
            break;
        case CRYPTO_TYPE_AES128:
            //CryptoAESEncryptBuffer(newBlockSize, bufferOut, &bufferOut[bufferSize], keyData, bufferIV);
            CryptoAESEncryptBuffer(newBlockSize, bufferOut, &bufferOut[bufferSize], bufferIV, keyData);
            break;
        default:
            return false;
    }
    memmove(&bufferOut[0], &bufferOut[newBlockSize], newBlockSize);
    memcpy(_cmac_final, &bufferOut[newBlockSize - blockSize], blockSize);
    memcpy(&bufferOut[newBlockSize], _cmac_final, blockSize);
    return true;
}

//TODO: This did not pass my tests -tomaspre
bool appendBufferCMAC(uint8_t cryptoType, const uint8_t *keyData, uint8_t *bufferData, uint16_t bufferSize, uint8_t *IV) {
    uint8_t blockSize, rb;
    uint8_t *nistL = _cmac_K2;
    switch (cryptoType) {
        case CRYPTO_TYPE_3K3DES:
            blockSize = CRYPTO_3KTDEA_BLOCK_SIZE;
            rb = CRYPTO_CMAC_RB64;
            memset(_cmac_zeros, 0x00, blockSize);
            Encrypt3DESBuffer(blockSize, _cmac_zeros, nistL, _cmac_zeros, keyData);
            break;
        case CRYPTO_TYPE_AES128:
            blockSize = CRYPTO_AES_BLOCK_SIZE;
            rb = CRYPTO_CMAC_RB128;
            memset(_cmac_zeros, 0x00, blockSize);
            CryptoAESEncryptBuffer(blockSize, _cmac_zeros, nistL, _cmac_zeros, keyData);
            break;
        default:
            return false;
    }
    getCMACSubK1(nistL, blockSize, rb, _cmac_K1);
    getCMACSubK2(_cmac_K1, blockSize, rb, _cmac_K2);
    if (IV == NULL) {
        IV = _cmac_zeros;
        memset(IV, 0x00, blockSize);
        return appendBufferCMACSubroutine(cryptoType, keyData, _cmac_K1, _cmac_K2, IV, blockSize, bufferData, bufferSize);
    } else {
        return appendBufferCMACSubroutine(cryptoType, keyData, _cmac_K1, _cmac_K2, IV, blockSize, bufferData, bufferSize);
    }
}

void bin_xor(uint8_t *d1, const uint8_t *d2, size_t len) {
    for (size_t i = 0; i < len; i++)
        d1[i] = d1[i] ^ d2[i];
}

void lsl(uint8_t *data, size_t len) {
    for (size_t n = 0; n < len - 1; n++) {
        data[n] = (data[n] << 1) | (data[n + 1] >> 7);
    }
    data[len - 1] <<= 1;
}

//Taken from https://github.com/RfidResearchGroup/proxmark3/blob/master/client/src/mifare/desfirecrypto.c
bool DesfireCMACGenerateSubkeys(uint8_t cryptoType, const uint8_t *keyData, uint8_t *sk1, uint8_t *sk2) {
    int kbs;
    uint8_t l[2*CRYPTO_MAX_BLOCK_SIZE];
    uint8_t ivect[CRYPTO_MAX_BLOCK_SIZE];

    switch (cryptoType) {
        case CRYPTO_TYPE_3K3DES:
            kbs = CRYPTO_3KTDEA_BLOCK_SIZE;
            break;
        case CRYPTO_TYPE_AES128:
            kbs = CRYPTO_AES_BLOCK_SIZE;
            break;
        default:
            return false;
    }

    const uint8_t R = (kbs == 8) ? 0x1B : 0x87;

    memset(l, 0, kbs);
    memset(ivect, 0, kbs);

    switch (cryptoType) {
        case CRYPTO_TYPE_3K3DES:
            Encrypt3DESBuffer(kbs, l, l+kbs, ivect, keyData);
            break;
        case CRYPTO_TYPE_AES128:
            CryptoAESEncryptBuffer(kbs, l, l+kbs, ivect, keyData);
            break;
        default:
            return false;
    }

    memcpy(l, l+kbs, kbs);

    bool txor = false;

    // Used to compute CMAC on complete blocks
    memcpy(sk1, l, kbs);
    txor = l[0] & 0x80;
    lsl(sk1, kbs);
    if (txor) {
        sk1[kbs - 1] ^= R;
    }

    // Used to compute CMAC on the last block if non-complete
    memcpy(sk2, sk1, kbs);
    txor = sk1[0] & 0x80;
    lsl(sk2, kbs);
    if (txor) {
        sk2[kbs - 1] ^= R;
    }

    return true;
}

bool DesfireCryptoCMAC(uint8_t cryptoType, const uint8_t *keyData, uint8_t *bufferDataIn, uint16_t bufferSize, uint8_t *IV, uint8_t *cmac){
    return DesfireCryptoCMACEx(cryptoType, keyData, bufferDataIn, bufferSize, IV, cmac, 0);
}

//Taken from https://github.com/RfidResearchGroup/proxmark3/blob/master/client/src/mifare/desfirecrypto.c
bool DesfireCryptoCMACEx(uint8_t cryptoType, const uint8_t *keyData, uint8_t *bufferDataIn, uint16_t bufferSize, uint8_t *IV, uint8_t *cmac, uint16_t minlen) {
    uint8_t kbs;
    uint8_t len = bufferSize;
    uint8_t * bufferData = bufferDataIn + bufferSize;
    memcpy(bufferData, bufferDataIn, bufferSize);

    switch (cryptoType) {
        case CRYPTO_TYPE_3K3DES:
            kbs = CRYPTO_3KTDEA_BLOCK_SIZE;
            break;
        case CRYPTO_TYPE_AES128:
            kbs = CRYPTO_AES_BLOCK_SIZE;
            break;
        default:
            return false;
    }


    uint8_t sk1[24] = {0};
    uint8_t sk2[24] = {0};

    DesfireCMACGenerateSubkeys(cryptoType, keyData, sk1, sk2);

    if ((!len) || (len % kbs) || (len < minlen)) {
        bufferData[len++] = 0x80;
        while (len % kbs || len < minlen) {
            bufferData[len++] = 0x00;
        }
        bin_xor(bufferData + len - kbs, sk2, kbs);
    } else {
        bin_xor(bufferData + len - kbs, sk1, kbs);
    }

    bool retval = false;

    switch (cryptoType) {
        case CRYPTO_TYPE_3K3DES:
            retval = Encrypt3DESBuffer(len, bufferData, bufferData+len, IV, keyData);
            break;
        case CRYPTO_TYPE_AES128:
            retval = CryptoAESEncryptBuffer(len, bufferData, bufferData+len, IV, keyData);
            break;
        default:
            return false;
    }

    if (cmac != NULL) {
        memcpy(cmac, IV, kbs);
        return retval;
    }

    return false;

}



bool checkBufferCMAC(uint8_t *bufferData, uint16_t bufferSize, uint16_t checksumSize) {
    if (checksumSize > bufferSize) {
        return false;
    }
    if (checksumSize == CRYPTO_3KTDEA_BLOCK_SIZE) {
        appendBufferCMAC(CRYPTO_TYPE_3K3DES, SessionKey, &bufferData[bufferSize], bufferSize - checksumSize, SessionIV);
        memcpy(_cmac_zeros, &bufferData[bufferSize], checksumSize);
        if (memcmp(_cmac_zeros, &bufferData[bufferSize - checksumSize], checksumSize)) {
            return false;
        }
        return true;
    } else if (checksumSize == CRYPTO_AES_BLOCK_SIZE) {
        appendBufferCMAC(CRYPTO_TYPE_AES128, SessionKey, &bufferData[bufferSize], bufferSize - checksumSize, SessionIV);
        memcpy(_cmac_zeros, &bufferData[bufferSize], checksumSize);
        if (memcmp(_cmac_zeros, &bufferData[bufferSize - checksumSize], checksumSize)) {
            return false;
        }
        return true;
    }
    return false;
}

uint16_t appendBufferMAC(const uint8_t *keyData, uint8_t *bufferData, uint16_t bufferSize) {
    memcpy(&_mac_key24[2 * CRYPTO_DES_BLOCK_SIZE], keyData, CRYPTO_DES_BLOCK_SIZE);
    memcpy(&_mac_key24[CRYPTO_DES_BLOCK_SIZE], keyData, CRYPTO_DES_BLOCK_SIZE);
    memcpy(&_mac_key24[0], keyData, CRYPTO_DES_BLOCK_SIZE);
    memset(&_cmac_zeros[0], 0x00, CRYPTO_DES_BLOCK_SIZE);
    uint16_t paddedBufferSize = bufferSize;
    if ((bufferSize % CRYPTO_DES_BLOCK_SIZE) != 0) {
        paddedBufferSize = bufferSize + CRYPTO_DES_BLOCK_SIZE - (bufferSize % CRYPTO_DES_BLOCK_SIZE);
        memset(&bufferData[bufferSize], 0x00, paddedBufferSize - bufferSize);
    }
    Encrypt3DESBuffer(paddedBufferSize, bufferData, &bufferData[paddedBufferSize], _cmac_zeros, keyData);
    // Copy the 4-byte MAC from the ciphertext (end of the bufferData array):
    memcpy(&_cmac_zeros[0], &bufferData[paddedBufferSize - CRYPTO_DES_BLOCK_SIZE], 4);
    memcpy(&bufferData[bufferSize], &_cmac_zeros[0], 4);
    return bufferSize + 4;
}

bool checkBufferMAC(uint8_t *bufferData, uint16_t bufferSize, uint16_t checksumSize) {
    if (checksumSize > bufferSize) {
        return false;
    }
    const uint8_t *keyData = SessionKey;
    memcpy(&_mac_key24[2 * CRYPTO_DES_BLOCK_SIZE], keyData, CRYPTO_DES_BLOCK_SIZE);
    memcpy(&_mac_key24[CRYPTO_DES_BLOCK_SIZE], keyData, CRYPTO_DES_BLOCK_SIZE);
    memcpy(&_mac_key24[0], keyData, CRYPTO_DES_BLOCK_SIZE);
    memset(&_cmac_zeros[0], 0x00, CRYPTO_DES_BLOCK_SIZE);
    uint16_t paddedBufferSize = bufferSize;
    if ((bufferSize % CRYPTO_DES_BLOCK_SIZE) != 0) {
        paddedBufferSize = bufferSize + CRYPTO_DES_BLOCK_SIZE - (bufferSize % CRYPTO_DES_BLOCK_SIZE);
        memset(&bufferData[bufferSize], 0x00, paddedBufferSize - bufferSize);
    }
    Encrypt3DESBuffer(paddedBufferSize, bufferData, &bufferData[paddedBufferSize], _cmac_zeros, keyData);
    // Copy the 4-byte MAC from the ciphertext (end of the bufferData array):
    memcpy(&_cmac_zeros[0], &bufferData[paddedBufferSize - CRYPTO_DES_BLOCK_SIZE], 4);
    if (memcmp(&bufferData[bufferSize - checksumSize], &_cmac_zeros[0], 4)) {
        return false;
    }
    return true;
}
