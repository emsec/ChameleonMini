/* CryptoTDEA.c : Extended support for the DES encryption schemes.
 * Author: Maxie D. Schmidt (@maxieds)
 */

#include <string.h>

#include "../Common.h"
#include "CryptoTDEA.h"
#include "CryptoAES128.h"

void EncryptDESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncryptDEA,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint16_t blockIndex = 0;
    uint8_t *ctBuf = (uint8_t *) Ciphertext;
    uint8_t inputBlock[CRYPTO_DES_BLOCK_SIZE];
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    while (blockIndex < numBlocks) {
        if (blockIndex == 0) {
            memcpy(inputBlock, &Plaintext[0], CRYPTO_DES_BLOCK_SIZE);
            CryptoMemoryXOR(IV, inputBlock, CRYPTO_DES_BLOCK_SIZE);
        } else {
            memcpy(inputBlock, &Ciphertext[(blockIndex - 1) * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
            CryptoMemoryXOR(&Plaintext[blockIndex * CRYPTO_DES_BLOCK_SIZE], inputBlock, CRYPTO_DES_BLOCK_SIZE);
        }
        CryptoSpec.cryptFunc(inputBlock, ctBuf, Keys);
        ctBuf += CryptoSpec.blockSize;
        blockIndex++;
    }
}

void DecryptDESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecryptDEA,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint16_t blockIndex = 0;
    uint8_t inputBlock[CRYPTO_DES_BLOCK_SIZE];
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    while (blockIndex < numBlocks) {
        CryptoSpec.cryptFunc(inputBlock, Ciphertext + blockIndex * CRYPTO_DES_BLOCK_SIZE, Keys);
        if (blockIndex == 0) {
            memcpy(Plaintext, inputBlock, CRYPTO_DES_BLOCK_SIZE);
            CryptoMemoryXOR(IV, Plaintext, CRYPTO_DES_BLOCK_SIZE);
        } else {
            memcpy(Plaintext + blockIndex * CRYPTO_DES_BLOCK_SIZE, inputBlock, CRYPTO_DES_BLOCK_SIZE);
            CryptoMemoryXOR(&Ciphertext[(blockIndex - 1) * CRYPTO_DES_BLOCK_SIZE],
                            Plaintext + blockIndex * CRYPTO_DES_BLOCK_SIZE, CRYPTO_DES_BLOCK_SIZE);
        }
        blockIndex++;
    }
}

void Encrypt3DESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint8_t *ctBuf = (uint8_t *) Ciphertext;
    uint16_t blockIndex = 0;
    uint8_t inputBlock[CRYPTO_3KTDEA_BLOCK_SIZE];
    uint8_t IV[CRYPTO_3KTDEA_BLOCK_SIZE];
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_3KTDEA_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_3KTDEA_BLOCK_SIZE);
    }
    while (blockIndex < numBlocks) {
        if (blockIndex == 0) {
            memcpy(inputBlock, &Plaintext[0], CRYPTO_3KTDEA_BLOCK_SIZE);
            CryptoMemoryXOR(IV, inputBlock, CRYPTO_3KTDEA_BLOCK_SIZE);
        } else {
            memcpy(inputBlock, &Ciphertext[(blockIndex - 1) * CRYPTO_3KTDEA_BLOCK_SIZE], CRYPTO_3KTDEA_BLOCK_SIZE);
            CryptoMemoryXOR(&Plaintext[blockIndex * CRYPTO_3KTDEA_BLOCK_SIZE], inputBlock, CRYPTO_3KTDEA_BLOCK_SIZE);
        }
        CryptoSpec.cryptFunc(inputBlock, ctBuf, Keys);
        ctBuf += CryptoSpec.blockSize;
        blockIndex++;
    }
}

void Decrypt3DESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint16_t blockIndex = 0;
    uint8_t inputBlock[CRYPTO_3KTDEA_BLOCK_SIZE];
    uint8_t IV[CRYPTO_3KTDEA_BLOCK_SIZE];
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_3KTDEA_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_3KTDEA_BLOCK_SIZE);
    }
    while (blockIndex < numBlocks) {
        CryptoSpec.cryptFunc(inputBlock, Ciphertext + blockIndex * CRYPTO_3KTDEA_BLOCK_SIZE, Keys);
        if (blockIndex == 0) {
            memcpy(Plaintext, inputBlock, CRYPTO_3KTDEA_BLOCK_SIZE);
            CryptoMemoryXOR(IV, Plaintext, CRYPTO_3KTDEA_BLOCK_SIZE);
        } else {
            memcpy(Plaintext + blockIndex * CRYPTO_3KTDEA_BLOCK_SIZE, inputBlock, CRYPTO_3KTDEA_BLOCK_SIZE);
            CryptoMemoryXOR(&Ciphertext[(blockIndex - 1) * CRYPTO_3KTDEA_BLOCK_SIZE],
                            Plaintext + blockIndex * CRYPTO_3KTDEA_BLOCK_SIZE, CRYPTO_3KTDEA_BLOCK_SIZE);
        }
        blockIndex++;
    }
}
