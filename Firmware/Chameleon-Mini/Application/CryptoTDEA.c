/* CryptoTDEA.c : Extended support for the DES encryption schemes.
 * Author: Maxie D. Schmidt (@maxieds)
 */

#include <string.h>

#include "../Common.h"
#include "CryptoTDEA.h"
#include "CryptoAES128.h"

/* Set the last operation mode (ECB or CBC) init for the context */
uint8_t __CryptoDESOpMode = CRYPTO_DES_ECB_MODE;

static int CryptoEncryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys);
static int CryptoEncryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    uint16_t numBlocks = (Count + CryptoSpec->blockSize - 1) / CryptoSpec->blockSize;
    bool unevenBlockSize = (Count % CryptoSpec->blockSize) != 0;
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
            } else if (unevenBlockSize && blockIndex + 1 == numBlocks) {
                memset(inputBlock, 0x00, CryptoSpec->blockSize);
                memcpy(inputBlock, &Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize], Count % CryptoSpec->blockSize);
                CryptoMemoryXOR(&Plaintext[blockIndex * CryptoSpec->blockSize], inputBlock, CryptoSpec->blockSize);
            } else {
                memcpy(inputBlock, &Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize], CryptoSpec->blockSize);
                CryptoMemoryXOR(&Plaintext[blockIndex * CryptoSpec->blockSize], inputBlock, CryptoSpec->blockSize);
            }
            CryptoSpec->cryptFunc(inputBlock, &Ciphertext[blockIndex * CryptoSpec->blockSize], Keys);
        } else { /* ECB mode */
            if (unevenBlockSize && blockIndex + 1 == numBlocks) {
                memset(inputBlock, 0x00, CryptoSpec->blockSize);
                memcpy(inputBlock, &Plaintext[blockIndex * CryptoSpec->blockSize], Count % CryptoSpec->blockSize);
            } else {
                memcpy(inputBlock, &Plaintext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
            }
            CryptoMemoryXOR(IV, inputBlock, CryptoSpec->blockSize);
            CryptoSpec->cryptFunc(inputBlock, &Ciphertext[blockIndex * CryptoSpec->blockSize], Keys);
            memcpy(IV, &Ciphertext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
        }
        blockIndex++;
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CryptoSpec->blockSize);
    }
    /* TODO: Check for status errors with AVR DES.STATUS ??? */
    if (unevenBlockSize) {
        return CRYPTO_TDEA_EXIT_UNEVEN_BLOCKS;
    } else {
        return CRYPTO_TDEA_EXIT_SUCCESS;
    }
}

static int CryptoDecryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys);
static int CryptoDecryptCBCBuffer(CryptoTDEA_CBCSpec *CryptoSpec, uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    uint16_t numBlocks = (Count + CryptoSpec->blockSize - 1) / CryptoSpec->blockSize;
    bool unevenBlockSize = (Count % CryptoSpec->blockSize) != 0;
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
            if (blockIndex == 0 && !unevenBlockSize) {
                memcpy(Plaintext, inputBlock, CryptoSpec->blockSize);
                CryptoMemoryXOR(IV, Plaintext, CryptoSpec->blockSize);
            } else if (blockIndex == 0 && unevenBlockSize && numBlocks == 0x01) {
                memcpy(Plaintext, inputBlock, Count % CryptoSpec->blockSize);
                CryptoMemoryXOR(IV, Plaintext, Count % CryptoSpec->blockSize);
            } else if (unevenBlockSize && blockIndex + 1 == numBlocks) {
                memcpy(Plaintext + blockIndex * CryptoSpec->blockSize, inputBlock, Count % CryptoSpec->blockSize);
                CryptoMemoryXOR(&Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize],
                                Plaintext + blockIndex * CryptoSpec->blockSize, Count % CryptoSpec->blockSize);
            } else {
                memcpy(Plaintext + blockIndex * CryptoSpec->blockSize, inputBlock, CryptoSpec->blockSize);
                CryptoMemoryXOR(&Ciphertext[(blockIndex - 1) * CryptoSpec->blockSize],
                                Plaintext + blockIndex * CryptoSpec->blockSize, CryptoSpec->blockSize);
            }
        } else { /* ECB mode */
            if (unevenBlockSize && blockIndex + 1 == numBlocks) {
                memset(inputBlock, 0x00, CryptoSpec->blockSize);
                memcpy(inputBlock, &Plaintext[blockIndex * CryptoSpec->blockSize], Count % CryptoSpec->blockSize);
            } else {
                memcpy(inputBlock, &Ciphertext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
            }
            CryptoSpec->cryptFunc(&Plaintext[blockIndex * CryptoSpec->blockSize], inputBlock, Keys);
            CryptoMemoryXOR(IV, &Plaintext[blockIndex * CryptoSpec->blockSize], CryptoSpec->blockSize);
            memcpy(IV, inputBlock, CryptoSpec->blockSize);
        }
        blockIndex++;
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CryptoSpec->blockSize);
    }
    /* TODO: Check for status errors with AVR DES.STATUS ??? */
    if (unevenBlockSize) {
        return CRYPTO_TDEA_EXIT_UNEVEN_BLOCKS;
    } else {
        return CRYPTO_TDEA_EXIT_SUCCESS;
    }
}

int EncryptDESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncryptDES,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    return CryptoEncryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

int DecryptDESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecryptDES,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    return CryptoDecryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

int Encrypt2K3DESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncrypt2KTDEA,
        .blockSize   = CRYPTO_2KTDEA_BLOCK_SIZE
    };
    return CryptoEncryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

int Decrypt2K3DESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecrypt2KTDEA,
        .blockSize   = CRYPTO_2KTDEA_BLOCK_SIZE
    };
    return CryptoDecryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

int Encrypt3DESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    return CryptoEncryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}

int Decrypt3DESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *IVIn, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    return CryptoDecryptCBCBuffer(&CryptoSpec, Count, Plaintext, Ciphertext, IVIn, Keys);
}
