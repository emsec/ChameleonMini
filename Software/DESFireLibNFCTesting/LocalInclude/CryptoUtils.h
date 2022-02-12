/* CryptoUtils.h */

#ifndef __CRYPTO_UTILS_H__
#define __CRYPTO_UTILS_H__

#include <string.h>
#include <stdint.h>

#include <openssl/des.h>
#include <openssl/rand.h>

#include <ArduinoCryptoLib-SingleSource.c>

#include "LibNFCUtils.h"

#define DESFIRE_CRYPTO_AUTHTYPE_AES128      (1)
#define DESFIRE_CRYPTO_AUTHTYPE_ISODES      (2)
#define DESFIRE_CRYPTO_AUTHTYPE_LEGACY      (3)

#define CRYPTO_DES_KEY_SIZE                 (8)
#define CRYPTO_3KTDEA_KEY_SIZE              (3 * CRYPTO_DES_KEY_SIZE)

#define CRYPTO_DES_BLOCK_SIZE               (8)
#define CRYPTO_3KTDEA_BLOCK_SIZE            (CRYPTO_DES_BLOCK_SIZE)
#define AES128_BLOCK_SIZE                   (16)

#define CRYPTO_CHALLENGE_RESPONSE_SIZE      (16)

static const inline uint8_t ZERO_KEY[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const inline uint8_t TEST_KEY1[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

static const inline uint8_t TEST_KEY1_INDEX = 0x01;

typedef AES128Context DesfireAESCryptoContext;

typedef struct {
    uint8_t  *keyData;
    size_t   keySize;
    uint8_t  *ivData;
    size_t   ivSize;
    DesfireAESCryptoContext cryptoCtx;
} CryptoData_t;

#define CryptoMemoryXOR(inputBuf, destBuf, bufSize) ({ \
     uint8_t *in = (uint8_t *) inputBuf;               \
     uint8_t *out = (uint8_t *) destBuf;               \
     uint16_t count = (uint16_t) bufSize;              \
     while(count-- > 0) {                              \
          *(out++) ^= *(in++);                         \
     }                                                 \
     })

static inline void DesfireAESCryptoInit(uint8_t *initKeyBuffer, uint16_t bufSize,
                                        DesfireAESCryptoContext *cryptoCtx) {
    if (initKeyBuffer == NULL || cryptoCtx == NULL) {
        return;
    }
    aes128InitContext(cryptoCtx);
    aes128SetKey(cryptoCtx, initKeyBuffer, bufSize);
}

static inline void PrintAESCryptoContext(DesfireAESCryptoContext *cryptoCtx) {
    if (cryptoCtx == NULL) {
        return;
    }
    fprintf(stdout, "    -- SCHED = ");
    print_hex(cryptoCtx->schedule, 16);
    fprintf(stdout, "    -- REV   = ");
    print_hex(cryptoCtx->reverse, 16);
    fprintf(stdout, "    -- KEY   = ");
    print_hex(cryptoCtx->keyData, 16);
}

static inline size_t EncryptAES128(const uint8_t *plainSrcBuf, size_t bufSize,
                                   uint8_t *encDestBuf, CryptoData_t cdata) {
    DesfireAESCryptoContext *cryptoCtx = &(cdata.cryptoCtx);
    DesfireAESCryptoInit(cdata.keyData, cdata.keySize, cryptoCtx);
    size_t bufBlocks = bufSize / AES128_BLOCK_SIZE;
    bool padLastBlock = (bufSize % AES128_BLOCK_SIZE) != 0;
    uint8_t IV[AES128_BLOCK_SIZE];
    memset(IV, 0x00, AES128_BLOCK_SIZE);
    for (int blk = 0; blk < bufBlocks; blk++) {
        uint8_t inputBlock[AES128_BLOCK_SIZE];
        if (blk == 0) {
            memcpy(inputBlock, &plainSrcBuf[0], AES128_BLOCK_SIZE);
            CryptoMemoryXOR(IV, inputBlock, AES128_BLOCK_SIZE);
        } else {
            memcpy(inputBlock, &encDestBuf[(blk - 1) * AES128_BLOCK_SIZE], AES128_BLOCK_SIZE);
            CryptoMemoryXOR(&plainSrcBuf[blk * AES128_BLOCK_SIZE], inputBlock, AES128_BLOCK_SIZE);
        }
        aes128EncryptBlock(cryptoCtx, encDestBuf + blk * AES128_BLOCK_SIZE,
                           inputBlock);
    }
    return bufSize;
}

static inline size_t DecryptAES128(const uint8_t *encSrcBuf, size_t bufSize,
                                   uint8_t *plainDestBuf, CryptoData_t cdata) {
    DesfireAESCryptoContext *cryptoCtx = &(cdata.cryptoCtx);
    DesfireAESCryptoInit(cdata.keyData, cdata.keySize, cryptoCtx);
    size_t bufBlocks = (bufSize + AES128_BLOCK_SIZE - 1) / AES128_BLOCK_SIZE;
    bool padLastBlock = (bufSize % AES128_BLOCK_SIZE) != 0;
    uint8_t IV[AES128_BLOCK_SIZE];
    memset(IV, 0x00, AES128_BLOCK_SIZE);
    for (int blk = 0; blk < bufBlocks; blk++) {
        uint8_t inputBlock[AES128_BLOCK_SIZE];
        aes128DecryptBlock(cryptoCtx, inputBlock, encSrcBuf + blk * AES128_BLOCK_SIZE);
        if (blk == 0) {
            memcpy(plainDestBuf + blk * AES128_BLOCK_SIZE, inputBlock, AES128_BLOCK_SIZE);
            CryptoMemoryXOR(IV, plainDestBuf + blk * AES128_BLOCK_SIZE, AES128_BLOCK_SIZE);
        } else {
            memcpy(plainDestBuf + blk * AES128_BLOCK_SIZE, inputBlock, AES128_BLOCK_SIZE);
            CryptoMemoryXOR(&encSrcBuf[(blk - 1) * AES128_BLOCK_SIZE],
                            plainDestBuf + blk * AES128_BLOCK_SIZE, AES128_BLOCK_SIZE);
        }
    }
    return bufSize;
}

static inline size_t Encrypt3DES(const uint8_t *plainSrcBuf, size_t bufSize,
                                 uint8_t *encDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched1, keySched2, keySched3;
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];
    uint8_t *kd1 = cdata.keyData, *kd2 = &(cdata.keyData[8]), *kd3 = &(cdata.keyData[16]);
    DES_set_key(kd1, &keySched1);
    DES_set_key(kd2, &keySched2);
    DES_set_key(kd3, &keySched3);
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    DES_ede3_cbc_encrypt(plainSrcBuf, encDestBuf, bufSize, &keySched1, &keySched2, &keySched3, &IV, DES_ENCRYPT);
    return bufSize;
}

static inline size_t Decrypt3DES(const uint8_t *encSrcBuf, size_t bufSize,
                                 uint8_t *plainDestBuf, const uint8_t *IVIn, CryptoData_t cdata) {
    DES_key_schedule keySched1, keySched2, keySched3;
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];
    uint8_t *kd1 = cdata.keyData, *kd2 = &(cdata.keyData[8]), *kd3 = &(cdata.keyData[16]);
    DES_set_key(kd1, &keySched1);
    DES_set_key(kd2, &keySched2);
    DES_set_key(kd3, &keySched3);
    if (IVIn == NULL) {
        memset(IV, 0x00, CRYPTO_DES_BLOCK_SIZE);
    } else {
        memcpy(IV, IVIn, CRYPTO_DES_BLOCK_SIZE);
    }
    DES_ede3_cbc_encrypt(encSrcBuf, plainDestBuf, bufSize, &keySched1, &keySched2, &keySched3, &IV, DES_DECRYPT);
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
    DES_cbc_encrypt(plainSrcBuf, encDestBuf, bufSize, &keySched, &IV, DES_ENCRYPT);
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
    DES_cbc_encrypt(encSrcBuf, plainDestBuf, bufSize, &keySched, &IV, DES_DECRYPT);
    return bufSize;
}


static inline bool TestAESEncyptionRoutines(void) {
    fprintf(stdout, ">>> TestAESEncryptionRoutines [non-DESFire command]:\n");
    const uint8_t keyData[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };
    const uint8_t ptData[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    const uint8_t ctData[] = {
        0xc8, 0xa3, 0x31, 0xff, 0x8e, 0xdd, 0x3d, 0xb1,
        0x75, 0xe1, 0x54, 0x5d, 0xbe, 0xfb, 0x76, 0x0b
    };
    CryptoData_t cdata;
    cdata.keyData = keyData;
    cdata.keySize = 16;
    uint8_t pt[16], pt2[16], ct[16];
    EncryptAES128(ptData, 16, ct, cdata);
    DecryptAES128(ct, 16, pt2, cdata);
    fprintf(stdout, "    -- : PT = ");
    print_hex(ptData, 16);
    fprintf(stdout, "    -- : CT = ");
    print_hex(ctData, 16);
    fprintf(stdout, "    -- : CT = ");
    print_hex(ct, 16);
    fprintf(stdout, "    -- : PT = ");
    print_hex(pt2, 16);
    bool status = true;
    if (memcmp(ct, ctData, 16)) {
        fprintf(stdout, "    -- CT does NOT match !!\n");
        status = false;
    } else {
        fprintf(stdout, "    -- CT matches.\n");
    }
    if (memcmp(pt2, ptData, 16)) {
        fprintf(stdout, "    -- Decrypted PT from CT does NOT match !!\n");
        status = false;
    } else {
        fprintf(stdout, "    -- Decrypted PT from CT matches.\n");
    }
    fprintf(stdout, "\n");
    return status;
}

static inline bool Test3DESEncyptionRoutines(void) {
    fprintf(stdout, ">>> Test3DESEncryptionRoutines [non-DESFire command]:\n");
    const uint8_t keyData[] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xf1, 0xe0, 0xd3, 0xc2, 0xb5, 0xa4, 0x97, 0x86,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    const uint8_t ptData[] = {
        0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x20,
        0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74,
        0x68, 0x65, 0x20, 0x74, 0x69, 0x6D, 0x65, 0x20,
        0x66, 0x6F, 0x72, 0x20, 0x00, 0x00, 0x00, 0x00
    };
    const uint8_t ctData[] = {
        0x3F, 0xE3, 0x01, 0xC9, 0x62, 0xAC, 0x01, 0xD0,
        0x22, 0x13, 0x76, 0x3C, 0x1C, 0xBD, 0x4C, 0xDC,
        0x79, 0x96, 0x57, 0xC0, 0x64, 0xEC, 0xF5, 0xD4,
        0x1C, 0x67, 0x38, 0x12, 0xCF, 0xDE, 0x96, 0x75
    };
    const uint8_t IV[] = {
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    CryptoData_t cdata;
    cdata.keyData = keyData;
    cdata.keySize = 3 * 8;
    const uint16_t testDataSize = 4 * 8;
    uint8_t pt[testDataSize], ct[testDataSize];
    Encrypt3DES(ptData, testDataSize, ct, IV, cdata);
    Decrypt3DES(ctData, testDataSize, pt, IV, cdata);
    fprintf(stdout, "    -- : PT [FIXED] = ");
    print_hex(ptData, testDataSize);
    fprintf(stdout, "    -- : CT [FIXED] = ");
    print_hex(ctData, testDataSize);
    fprintf(stdout, "    -- : CT [ENC]   = ");
    print_hex(ct, testDataSize);
    fprintf(stdout, "    -- : PT [DEC]   = ");
    print_hex(pt, testDataSize);
    bool status = true;
    if (memcmp(ct, ctData, testDataSize)) {
        fprintf(stdout, "    -- CT does NOT match !!\n");
        status = false;
    } else {
        fprintf(stdout, "    -- CT matches.\n");
    }
    if (memcmp(pt, ptData, testDataSize)) {
        fprintf(stdout, "    -- Decrypted PT from CT does NOT match !!\n");
        status = false;
    } else {
        fprintf(stdout, "    -- Decrypted PT from CT matches.\n");
    }
    fprintf(stdout, "\n");
    return status;
}

static inline bool TestLegacyDESEncyptionRoutines(void) {
    fprintf(stdout, ">>> TestLegacyDESEncryptionRoutines [non-DESFire command]:\n");
    const uint8_t keyData[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    };
    const uint8_t ptData[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    const uint8_t ctData[] = {
        0x3e, 0xf0, 0xa8, 0x91, 0xcf, 0x8e, 0xd9, 0x90,
        0xc4, 0x77, 0xeb, 0x09, 0x02, 0xf0, 0xc5, 0x4a
    };
    CryptoData_t cdata;
    cdata.keyData = keyData;
    cdata.keySize = 8;
    const uint16_t testDataSize = 2 * 8;
    uint8_t pt[testDataSize], ct[testDataSize];
    EncryptDES(ptData, testDataSize, ct, NULL, cdata);
    DecryptDES(ctData, testDataSize, pt, NULL, cdata);
    fprintf(stdout, "    -- : PT [FIXED] = ");
    print_hex(ptData, testDataSize);
    fprintf(stdout, "    -- : CT [FIXED] = ");
    print_hex(ctData, testDataSize);
    fprintf(stdout, "    -- : CT [ENC]   = ");
    print_hex(ct, testDataSize);
    fprintf(stdout, "    -- : PT [DEC]   = ");
    print_hex(pt, testDataSize);
    bool status = true;
    if (memcmp(ct, ctData, testDataSize)) {
        fprintf(stdout, "    -- CT does NOT match !!\n");
        status = false;
    } else {
        fprintf(stdout, "    -- CT matches.\n");
    }
    if (memcmp(pt, ptData, testDataSize)) {
        fprintf(stdout, "    -- Decrypted PT from CT does NOT match !!\n");
        status = false;
    } else {
        fprintf(stdout, "    -- Decrypted PT from CT matches.\n");
    }
    fprintf(stdout, "\n");
    return status;
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
    for (int bidx = 1; bidx < bufSize; bidx++) {
        destBuf[bidx] = srcBuf[bidx - 1];
    }
    destBuf[0] = srcBuf[bufSize - 1];
}

static inline void ConcatByteArrays(uint8_t *arrA, size_t arrASize,
                                    uint8_t *arrB, size_t arrBSize, uint8_t *destArr) {
    memcpy(destArr, arrA, arrASize);
    memcpy(destArr + arrASize, arrB, arrBSize);
}

#endif
