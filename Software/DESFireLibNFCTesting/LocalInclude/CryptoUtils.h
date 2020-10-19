/* CryptoUtils.h */

#ifndef __CRYPTO_UTILS_H__
#define __CRYPTO_UTILS_H__

#include <string.h>
#include <stdint.h>

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
     if(initKeyBuffer == NULL || cryptoCtx == NULL) {
          return;
     }   
     aes128InitContext(cryptoCtx);
     aes128SetKey(cryptoCtx, initKeyBuffer, bufSize);
}

static inline void PrintAESCryptoContext(DesfireAESCryptoContext *cryptoCtx) {
    if(cryptoCtx == NULL) {
        return;
    }
    fprintf(stdout, "    -- SCHED = "); print_hex(cryptoCtx->schedule, 16);
    fprintf(stdout, "    -- REV   = "); print_hex(cryptoCtx->reverse, 16);
    fprintf(stdout, "    -- KEY   = "); print_hex(cryptoCtx->keyData, 16);
}

static inline size_t EncryptAES128(const uint8_t *plainSrcBuf, size_t bufSize, 
                                   uint8_t *encDestBuf, CryptoData_t cdata) {
     DesfireAESCryptoContext *cryptoCtx = &(cdata.cryptoCtx);
     DesfireAESCryptoInit(cdata.keyData, cdata.keySize, cryptoCtx);
     size_t bufBlocks = bufSize / AES128_BLOCK_SIZE;
     bool padLastBlock = (bufSize % AES128_BLOCK_SIZE) != 0;
     uint8_t IV[AES128_BLOCK_SIZE];
     memset(IV, 0x00, AES128_BLOCK_SIZE);
     for(int blk = 0; blk < bufBlocks; blk++) {
          uint8_t inputBlock[AES128_BLOCK_SIZE];
          if(blk == 0) {
               memcpy(inputBlock, &plainSrcBuf[0], AES128_BLOCK_SIZE);
               CryptoMemoryXOR(IV, inputBlock, AES128_BLOCK_SIZE);
          }   
          else {
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
     for(int blk = 0; blk < bufBlocks; blk++) {
          uint8_t inputBlock[AES128_BLOCK_SIZE];
          aes128DecryptBlock(cryptoCtx, inputBlock, encSrcBuf + blk * AES128_BLOCK_SIZE);
          if(blk == 0) {
               memcpy(plainDestBuf + blk * AES128_BLOCK_SIZE, inputBlock, AES128_BLOCK_SIZE);
               CryptoMemoryXOR(IV, plainDestBuf + blk * AES128_BLOCK_SIZE, AES128_BLOCK_SIZE);
          }
          else {
               memcpy(plainDestBuf + blk * AES128_BLOCK_SIZE, inputBlock, AES128_BLOCK_SIZE);
               CryptoMemoryXOR(&encSrcBuf[(blk - 1) * AES128_BLOCK_SIZE],
                               plainDestBuf + blk * AES128_BLOCK_SIZE, AES128_BLOCK_SIZE);
          }
     }
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
    fprintf(stdout, "    -- : PT = "); print_hex(ptData, 16);
    fprintf(stdout, "    -- : CT = "); print_hex(ctData, 16);
    fprintf(stdout, "    -- : CT = "); print_hex(ct, 16);
    fprintf(stdout, "    -- : PT = "); print_hex(pt2, 16);
    bool status = true;
    if(memcmp(ct, ctData, 16)) {
        fprintf(stdout, "    -- CT does NOT match !!\n");
        status = false;
    }
    else {
        fprintf(stdout, "    -- CT matches.\n");
    }
    if(memcmp(pt2, ptData, 16)) {
        fprintf(stdout, "    -- Decrypted PT from CT does NOT match !!\n");
        status = false;
    }
    else {
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
     for(int bidx = 0; bidx < bufSize - 1; bidx++) {
          destBuf[bidx] = srcBuf[bidx + 1];
     }
}

static inline void RotateArrayLeft(uint8_t *srcBuf, uint8_t *destBuf, size_t bufSize) {
     for(int bidx = 1; bidx < bufSize; bidx++) {
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
