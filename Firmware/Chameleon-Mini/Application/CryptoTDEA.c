/* CryptoTDEA.c : Extended support for the DES encryption schemes.
 * Author: Maxie D. Schmidt (@maxieds)
 */

#include <string.h>

#include "../Common.h"
#include "CryptoTDEA.h"
#include "CryptoAES128.h"

/* Set the last operation mode (ECB or CBC) init for the context */
uint8_t __CryptoDESOpMode = CRYPTO_DES_ECB_MODE;

static void CryptoEncryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys);
static void CryptoEncryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    uint16_t numBlocks = (Count + CryptoSpec->blockSize - 1) / CryptoSpec->blockSize;
    bool dataNeedsPadding = (Count % CryptoSpec->blockSize) != 0;
    uint16_t blockIndex = 0;
    uint8_t inputBlock[CryptoSpec->blockSize];
    uint8_t IV[CryptoSpec->blockSize];
    if (IVIn == NULL) {
        memset(IV, 0x00, CryptoSpec->blockSize);
    } else {
        memcpy(IV, IVIn, CryptoSpec->blockSize);
    }
    while (blockIndex < numBlocks) {
        if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
            if (blockIndex == 0) {
                memset(inputBlock, 0x00, CryptoSpec->blockSize);
                memcpy(inputBlock, &Plaintext[0], CryptoSpec->blockSize);
                CryptoMemoryXOR(IV, inputBlock, CryptoSpec->blockSize);
            } else if (dataNeedsPadding && blockIndex + 1 == numBlocks) {
                memset(inputBlock, 0x00, CryptoSpec->blockSize);
                memcpy(inputBlock, &Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize], Count % CryptoSpec->blockSize);
                CryptoMemoryXOR(&Plaintext[blockIndex * CryptoSpec->blockSize], inputBlock, CryptoSpec->blockSize);
            } else {
                memcpy(inputBlock, &Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize], CryptoSpec->blockSize);
                CryptoMemoryXOR(&Plaintext[blockIndex * CryptoSpec->blockSize], inputBlock, CryptoSpec->blockSize);
            }
            CryptoSpec->cryptFunc(inputBlock, &Ciphertext[blockIndex * CryptoSpec->blockSize], Keys);
        } else { /* ECB mode */
            memcpy(inputBlock, &Plaintext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
            CryptoMemoryXOR(IV, inputBlock, CryptoSpec->blockSize);
            CryptoSpec->cryptFunc(inputBlock, &Ciphertext[blockIndex * CryptoSpec->blockSize], Keys);
            memcpy(IV, &Ciphertext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
        }
        blockIndex++;
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CryptoSpec->blockSize);
    }
}

static void CryptoDecryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys);
static void CryptoDecryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    uint16_t numBlocks = (Count + CryptoSpec->blockSize - 1) / CryptoSpec->blockSize;
    bool dataNeedsPadding = (Count % CryptoSpec->blockSize) != 0;
    uint16_t blockIndex = 0;
    uint8_t inputBlock[CryptoSpec->blockSize];
    uint8_t IV[CryptoSpec->blockSize];
    if (IVIn == NULL) {
        memset(IV, 0x00, CryptoSpec->blockSize);
    } else {
        memcpy(IV, IVIn, CryptoSpec->blockSize);
    }
    while (blockIndex < numBlocks) {
        if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
            CryptoSpec->cryptFunc(inputBlock, Ciphertext + blockIndex * CryptoSpec->blockSize, Keys);
            if (blockIndex == 0 && !dataNeedsPadding) {
                memcpy(Plaintext, inputBlock, CryptoSpec->blockSize);
                CryptoMemoryXOR(IV, Plaintext, CryptoSpec->blockSize);
            } else if (blockIndex == 0 && dataNeedsPadding && numBlocks == 0x01) {
                memcpy(Plaintext, inputBlock, Count % CryptoSpec->blockSize);
                CryptoMemoryXOR(IV, Plaintext, Count % CryptoSpec->blockSize);
            } else if (dataNeedsPadding && blockIndex + 1 == numBlocks) {
                memcpy(Plaintext + blockIndex * CryptoSpec->blockSize, inputBlock, Count % CryptoSpec->blockSize);
                CryptoMemoryXOR(&Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize],
                                Plaintext + blockIndex * CryptoSpec->blockSize, Count % CryptoSpec->blockSize);
            } else {
                memcpy(Plaintext + blockIndex * CryptoSpec->blockSize, inputBlock, CryptoSpec->blockSize);
                CryptoMemoryXOR(&Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize],
                                Plaintext + blockIndex * CryptoSpec->blockSize, CryptoSpec->blockSize);
            }
        } else { /* ECB mode */
            CryptoSpec->cryptFunc(&Plaintext[blockIndex * CryptoSpec->blockSize],
                                  &Ciphertext[blockIndex * CryptoSpec->blockSize], Keys);
            CryptoMemoryXOR(IV, &Plaintext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
            memcpy(IV, &Ciphertext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
        }
        blockIndex++;
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CryptoSpec->blockSize);
    }
}

void EncryptDESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncryptDEA,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    CryptoEncryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

void DecryptDESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecryptDEA,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    CryptoDecryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

void Encrypt2K3DESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncrypt2KTDEA,
        .blockSize   = CRYPTO_2KTDEA_BLOCK_SIZE
    };
    CryptoEncryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

void Decrypt2K3DESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecrypt2KTDEA,
        .blockSize   = CRYPTO_2KTDEA_BLOCK_SIZE
    };
    CryptoDecryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

void Encrypt3DESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    CryptoEncryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

void Decrypt3DESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    CryptoDecryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}
