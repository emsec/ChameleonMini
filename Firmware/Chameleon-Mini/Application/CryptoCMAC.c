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

static void getCMACSubK1(const uint8_t *bufferL, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut);
static void getCMACSubK1(const uint8_t *bufferL, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut) {
    _cmac_RB[blockSize - 1] = polyByte;
    RotateArrayLeft(bufferL, bufferOut, blockSize);
    if ((bufferL[0] & 0x80) != 0) {
        for (int kidx = 0; kidx < blockSize; kidx++) {
            bufferOut[kidx] = (uint8_t) (bufferOut[kidx] ^ _cmac_RB[kidx]);
	}
    }
}

static void getCMACSubK2(const uint8_t *bufferK1, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut);
static void getCMACSubK2(const uint8_t *bufferK1, uint8_t blockSize, uint8_t polyByte, uint8_t *bufferOut) {
    _cmac_RB[blockSize - 1] = polyByte;
    RotateArrayLeft(bufferK1, bufferOut, blockSize);
    if ((bufferK1[0] & 0x80) != 0) {
        for (int kidx = 0; kidx < blockSize; kidx++) {
            bufferOut[kidx] = (uint8_t) (bufferOut[kidx] ^ _cmac_RB[kidx]);
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
            Encrypt3DESBuffer(newBlockSize, bufferOut, bufferOut + bufferSize, keyData, bufferIV);
	    break;
	case CRYPTO_TYPE_AES128:
            CryptoAESEncryptBuffer(newBlockSize, bufferOut, bufferOut + bufferSize, keyData, bufferIV);
	    break;
        default:
	    return false;
    }
    memmove(&bufferOut[0], &bufferOut[newBlockSize], newBlockSize);
    memcpy(_cmac_final, &bufferOut[newBlockSize - blockSize], blockSize);
    memcpy(&bufferOut[newBlockSize], _cmac_final, blockSize);
    return true;
}

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
