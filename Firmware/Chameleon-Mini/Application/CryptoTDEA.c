/* CryptoTDEA.c : Extended support for the DES encryption schemes.
 * Author: Maxie D. Schmidt (@maxieds)
 */

#include "../Common.h"
#include "CryptoTDEA.h"

//__asm__(
//    STRINGIFY(#include "CryptoTDEA.S")
//);

void EncryptDESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncryptDEA,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint16_t blockIndex = 0;
    uint8_t *ptBuf = (uint8_t *) Plaintext, *ctBuf = (uint8_t *) Ciphertext;
    while (blockIndex < numBlocks) {
        CryptoSpec.cryptFunc(ptBuf, ctBuf, Keys);
        ptBuf += CryptoSpec.blockSize;
        ctBuf += CryptoSpec.blockSize;
        blockIndex++;
    }
}

void DecryptDESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecryptDEA,
        .blockSize   = CRYPTO_DES_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint16_t blockIndex = 0;
    uint8_t *ptBuf = (uint8_t *) Plaintext, *ctBuf = (uint8_t *) Ciphertext;
    while (blockIndex < numBlocks) {
        CryptoSpec.cryptFunc(ptBuf, ctBuf, Keys);
        ptBuf += CryptoSpec.blockSize;
        ctBuf += CryptoSpec.blockSize;
        blockIndex++;
    }
}

void Encrypt3DESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoEncrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint16_t blockIndex = 0;
    uint8_t *ptBuf = (uint8_t *) Plaintext, *ctBuf = (uint8_t *) Ciphertext;
    while (blockIndex < numBlocks) {
        CryptoSpec.cryptFunc(ptBuf, ctBuf, Keys);
        ptBuf += CryptoSpec.blockSize;
        ctBuf += CryptoSpec.blockSize;
        blockIndex++;
    }
}

void Decrypt3DESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *Keys) {
    CryptoTDEA_CBCSpec CryptoSpec = {
        .cryptFunc   = &CryptoDecrypt3KTDEA,
        .blockSize   = CRYPTO_3KTDEA_BLOCK_SIZE
    };
    uint16_t numBlocks = (Count + CryptoSpec.blockSize - 1) / CryptoSpec.blockSize;
    uint16_t blockIndex = 0;
    uint8_t *ptBuf = (uint8_t *) Plaintext, *ctBuf = (uint8_t *) Ciphertext;
    while (blockIndex < numBlocks) {
        CryptoSpec.cryptFunc(ptBuf, ctBuf, Keys);
        ptBuf += CryptoSpec.blockSize;
        ctBuf += CryptoSpec.blockSize;
        blockIndex++;
    }
}
