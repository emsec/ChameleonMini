/* CryptoUtils.h */

#ifndef __CRYPTO_UTILS_H__
#define __CRYPTO_UTILS_H__

#include <string.h>
#include <stdint.h>

#include <openssl/rand.h>
#include <openssl/des.h>

#include "LibNFCUtils.h"

/* DES Operation cipher modes: */
#define CRYPTO_DES_ECB_MODE                      0     ///< Electronic Code Book (ECB) mode
#define CRYPTO_DES_CBC_MODE                      1     ///< Cipher Block Chaining (CBC) mode

/* AES Operation cipher modes: */
#define CRYPTO_AES_ECB_MODE                      0     ///< Electronic Code Book (ECB) mode
#define CRYPTO_AES_CBC_MODE                      1     ///< Cipher Block Chaining (CBC) mode

#define DESFIRE_CRYPTO_AUTHTYPE_AES128          (1)
#define DESFIRE_CRYPTO_AUTHTYPE_ISODES          (2)
#define DESFIRE_CRYPTO_AUTHTYPE_LEGACY          (3)

#define CRYPTO_DES_BLOCK_SIZE                   (8)
#define CRYPTO_3KTDEA_BLOCK_SIZE                (CRYPTO_DES_BLOCK_SIZE)
#define AES128_BLOCK_SIZE                       (16)
#define CRYPTO_MAX_BLOCK_SIZE                   (AES128_BLOCK_SIZE)

#define CRYPTO_DES_KEY_SIZE                     (CRYPTO_DES_BLOCK_SIZE)
#define CRYPTO_3KTDEA_KEY_SIZE                  (3 * CRYPTO_DES_KEY_SIZE)
#define CRYPTO_AES128_KEY_SIZE                  (AES128_BLOCK_SIZE)

typedef uint8_t CryptoAESBlock_t[AES128_BLOCK_SIZE];
static uint8_t SessionIV[CRYPTO_MAX_BLOCK_SIZE];

/* Key sizes, block sizes (in bytes): */
#define CRYPTO_MAX_KEY_SIZE                     (3 * CRYPTO_DES_KEY_SIZE)
#define CRYPTO_MAX_BLOCK_SIZE                   (2 * CRYPTO_DES_KEY_SIZE)

#define CRYPTO_CHALLENGE_RESPONSE_SIZE          (AES128_BLOCK_SIZE)
#define CRYPTO_CHALLENGE_RESPONSE_SIZE_LEGACY   (CRYPTO_DES_BLOCK_SIZE)

static const uint8_t ZERO_KEY[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t TEST_KEY1[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

static const uint8_t TEST_KEY1_INDEX = 0x01;

typedef struct {
    uint8_t  *keyData;
    size_t   keySize;
    uint8_t  *ivData;
    size_t   ivSize;
} CryptoData_t;

#define CryptoMemoryXOR(inputBuf, destBuf, bufSize) ({ \
     uint8_t *in = (uint8_t *) inputBuf;               \
     uint8_t *out = (uint8_t *) destBuf;               \
     uint16_t count = (uint16_t) bufSize;              \
     while(count-- > 0) {                              \
          *(out++) ^= *(in++);                         \
     }                                                 \
     })


/* Set the last operation mode (ECB or CBC) init for the context */
static uint8_t __CryptoDESOpMode = CRYPTO_DES_ECB_MODE;

static inline size_t Encrypt2K3DES(const uint8_t *plainSrcBuf, size_t bufSize,
                                   uint8_t *encDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched1, keySched2;
    uint8_t *IV = SessionIV;
    uint8_t *kd1 = cdata.keyData, *kd2 = &(cdata.keyData[CRYPTO_DES_BLOCK_SIZE]);
    DES_set_key(kd1, &keySched1);
    DES_set_key(kd2, &keySched2);
    if (IVIn == NULL) {
        memset(IV, 0x00, 2 * CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, 2 * CRYPTO_DES_BLOCK_SIZE);
    }
    if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
        DES_ede2_cbc_encrypt(plainSrcBuf, encDestBuf, bufSize, &keySched1, &keySched2, &IV, DES_ENCRYPT);
    } else {
        uint8_t inputBlock[CRYPTO_DES_BLOCK_SIZE];
        uint16_t numBlocks = bufSize / CRYPTO_DES_BLOCK_SIZE;
        for (int blk = 0; blk < numBlocks; blk++) {
            memcpy(inputBlock, &plainSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
            CryptoMemoryXOR(IV, inputBlock, CRYPTO_DES_BLOCK_SIZE);
            DES_ecb2_encrypt(inputBlock, &encDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], &keySched1, &keySched2, DES_ENCRYPT);
            memcpy(IV, &encDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
        }
    }
    return bufSize;
}

static inline size_t Decrypt2K3DES(const uint8_t *encSrcBuf, size_t bufSize,
                                   uint8_t *plainDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched1, keySched2;
    uint8_t *IV = SessionIV;
    uint8_t *kd1 = cdata.keyData, *kd2 = &(cdata.keyData[CRYPTO_DES_BLOCK_SIZE]);
    DES_set_key(kd1, &keySched1);
    DES_set_key(kd2, &keySched2);
    if (IVIn == NULL) {
        memset(IV, 0x00, 2 * CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, 2 * CRYPTO_DES_BLOCK_SIZE);
    }
    if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
        DES_ede2_cbc_encrypt(encSrcBuf, plainDestBuf, bufSize, &keySched1, &keySched2, &IV, DES_DECRYPT);
    } else {
        uint8_t inputBlock[CRYPTO_DES_BLOCK_SIZE];
        uint16_t numBlocks = bufSize / CRYPTO_DES_BLOCK_SIZE;
        for (int blk = 0; blk < numBlocks; blk++) {
            DES_ecb2_encrypt(&encSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE],
                             &plainDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], &keySched1, &keySched2, DES_DECRYPT);
            CryptoMemoryXOR(IV, &plainDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
            memcpy(IV, &encSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
        }
    }
    return bufSize;
}

static inline size_t Encrypt3DES(const uint8_t *plainSrcBuf, size_t bufSize,
                                 uint8_t *encDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched1, keySched2, keySched3;
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE], inputBlock[CRYPTO_DES_BLOCK_SIZE];
    uint8_t *kd1 = cdata.keyData, *kd2 = &(cdata.keyData[CRYPTO_DES_BLOCK_SIZE]), *kd3 = &(cdata.keyData[2 * CRYPTO_DES_BLOCK_SIZE]);
    DES_set_key(kd1, &keySched1);
    DES_set_key(kd2, &keySched2);
    DES_set_key(kd3, &keySched3);
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
        DES_ede3_cbc_encrypt(plainSrcBuf, encDestBuf, bufSize, &keySched1, &keySched2, &keySched3, &IV, DES_ENCRYPT);
    } else {
        uint16_t numBlocks = bufSize / CRYPTO_DES_BLOCK_SIZE;
        for (int blk = 0; blk < numBlocks; blk++) {
            memcpy(inputBlock, &plainSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
            CryptoMemoryXOR(IV, inputBlock, CRYPTO_DES_BLOCK_SIZE);
            DES_ecb3_encrypt(inputBlock, &encDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], &keySched1, &keySched2, &keySched3, DES_ENCRYPT);
            memcpy(IV, &encDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
        }
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CRYPTO_DES_BLOCK_SIZE);
    }
    return bufSize;
}

static inline size_t Decrypt3DES(const uint8_t *encSrcBuf, size_t bufSize,
                                 uint8_t *plainDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched1, keySched2, keySched3;
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE], inputBlock[CRYPTO_DES_BLOCK_SIZE];
    uint8_t *kd1 = cdata.keyData, *kd2 = &(cdata.keyData[CRYPTO_DES_BLOCK_SIZE]), *kd3 = &(cdata.keyData[2 * CRYPTO_DES_BLOCK_SIZE]);
    DES_set_key(kd1, &keySched1);
    DES_set_key(kd2, &keySched2);
    DES_set_key(kd3, &keySched3);
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
        DES_ede3_cbc_encrypt(encSrcBuf, plainDestBuf, bufSize, &keySched1, &keySched2, &keySched3, &IV, DES_DECRYPT);
    } else {
        uint16_t numBlocks = bufSize / CRYPTO_DES_BLOCK_SIZE;
        for (int blk = 0; blk < numBlocks; blk++) {
            DES_ecb3_encrypt(&encSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE],
                             &plainDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], &keySched1, &keySched2, &keySched3, DES_DECRYPT);
            CryptoMemoryXOR(IV, &plainDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
            memcpy(IV, &encSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
        }
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CRYPTO_DES_BLOCK_SIZE);
    }
    return bufSize;
}


static inline size_t EncryptDES(const uint8_t *plainSrcBuf, size_t bufSize,
                                uint8_t *encDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched;
    uint8_t *kd = cdata.keyData;
    DES_set_key(kd, &keySched);
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
        DES_cbc_encrypt(plainSrcBuf, encDestBuf, bufSize, &keySched, &IV, DES_ENCRYPT);
    } else {
        uint8_t inputBlock[CRYPTO_DES_BLOCK_SIZE];
        uint16_t numBlocks = bufSize / CRYPTO_DES_BLOCK_SIZE;
        for (int blk = 0; blk < numBlocks; blk++) {
            memcpy(inputBlock, &plainSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
            CryptoMemoryXOR(IV, inputBlock, CRYPTO_DES_BLOCK_SIZE);
            DES_ecb_encrypt(inputBlock, &encDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], &keySched, DES_ENCRYPT);
            memcpy(IV, &encDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
        }
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CRYPTO_DES_BLOCK_SIZE);
    }
    return bufSize;
}

static inline size_t DecryptDES(const uint8_t *encSrcBuf, size_t bufSize,
                                uint8_t *plainDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched;
    uint8_t *kd = cdata.keyData;
    DES_set_key(kd, &keySched);
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    if (__CryptoDESOpMode == CRYPTO_DES_CBC_MODE) {
        DES_cbc_encrypt(encSrcBuf, plainDestBuf, bufSize, &keySched, &IV, DES_DECRYPT);
    } else {
        uint8_t inputBlock[CRYPTO_DES_BLOCK_SIZE];
        uint16_t numBlocks = bufSize / CRYPTO_DES_BLOCK_SIZE;
        for (int blk = 0; blk < numBlocks; blk++) {
            DES_ecb_encrypt(&encSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE],
                            &plainDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], &keySched, DES_DECRYPT);
            CryptoMemoryXOR(IV, &plainDestBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
            memcpy(IV, &encSrcBuf[blk * CRYPTO_DES_BLOCK_SIZE], CRYPTO_DES_BLOCK_SIZE);
        }
    }
    if (IVIn != NULL) {
        memcpy(IVIn, IV, CRYPTO_DES_BLOCK_SIZE);
    }
    return bufSize;
}

static inline int GenerateRandomBytes(uint8_t *destBuf, size_t numBytes) {
    return RAND_pseudo_bytes(destBuf, numBytes);
}

static inline void RotateArrayRight(uint8_t *srcBuf, uint8_t *destBuf, size_t bufSize) {
    destBuf[bufSize - 1] = srcBuf[0];
    for (int bidx = 0; bidx < bufSize - 1; bidx++) {
        destBuf[bidx] = srcBuf[bidx + 1];
    }
}

static inline void RotateArrayLeft(uint8_t *srcBuf, uint8_t *destBuf, size_t bufSize) {
    uint8_t lastDataByte = srcBuf[bufSize - 1];
    for (int bidx = 1; bidx < bufSize; bidx++) {
        destBuf[bidx] = srcBuf[bidx - 1];
    }
    destBuf[0] = lastDataByte;
}

static inline void ConcatByteArrays(uint8_t *arrA, size_t arrASize,
                                    uint8_t *arrB, size_t arrBSize, uint8_t *destArr) {
    memcpy(destArr, arrA, arrASize);
    memcpy(destArr + arrASize, arrB, arrBSize);
}

#endif
