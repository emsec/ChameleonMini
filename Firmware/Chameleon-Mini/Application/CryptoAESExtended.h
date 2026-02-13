/*
The DESFire stack portion of this firmware source
is free software written by Dr. Maxie Dion Schmidt (@maxieds):
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
 * CryptoAESExtended.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

/*
 * NOTE: These (two) source files implement software-based AES-192 and 
 *       AES-256 encryption routines. The source is heavily based on the 
 *       project (with the top level repo "unlicense.txt" license): 
 *       https://github.com/kokke/tiny-AES-c
 */

#if defined(CONFIG_MF_DESFIRE_SUPPORT) && defined(ENABLE_DESFIRE_AES_EXTENDED)

#ifndef __CRYPTO_AES_EXTENDED_SW_H__
#define __CRYPTO_AES_EXTENDED_SW_H__

#include "../Common.h"

#define _CRYPTO_TYPE_AES192        (0x8A)
#define _CRYPTO_TYPE_AES256        (0xDA)

#define AES_BLOCKLEN               (16)
#define CRYPTO_AES_MAX_KEY_EXPSIZE (240)

typedef struct {
    int Nk; // The number of 32 bit words in a key.
    int Nr; // The number of rounds in AES Cipher.
    int KeySize;
    int KeyExpSize;
    uint8_t CryptoMode;
    uint8_t *RoundKey;
    uint8_t *Iv;
} AES_ctx_t;

extern uint8_t AESCryptoType;
extern uint8_t Nk;
extern uint8_t Nr;
extern uint8_t AES_KEYLEN;
extern uint8_t AES_keyExpSize;
extern AES_ctx_t AES_ctx; 
extern uint8_t RoundKey[CRYPTO_AES_MAX_KEY_EXPSIZE];
extern uint8_t IV[AES_BLOCKLEN];
extern const uint8_t SBox[256];
extern const uint8_t RSBox[256];
extern const uint8_t RCon[11];

void SetupLocalAESContext(AES_ctx_t *ctx, uint8_t cryptoType, uint8_t cryptoMode);
uint8_t AESEncryptBuffer(const AES_ctx_t *ctx, uint8_t *buf, int bufLength);
uint8_t AESDecryptBuffer(const AES_ctx_t *ctx, uint8_t *buf, int bufLength);

void AES_init_ctx(AES_ctx_t* ctx, const uint8_t* key);
void AES_init_ctx_iv(AES_ctx_t* ctx, const uint8_t* key, const uint8_t* iv);
void AES_ctx_set_iv(AES_ctx_t* ctx, const uint8_t* iv);

// Buffer size is exactly AES_BLOCKLEN bytes. 
// You need only AES_init_ctx as IV is not used in ECB.
// NB: ECB is considered insecure for most uses.
void AES_ECB_encrypt(const AES_ctx_t* ctx, uint8_t* buf);
void AES_ECB_decrypt(const AES_ctx_t* ctx, uint8_t* buf);

// Buffer size MUST be mutile of AES_BLOCKLEN.
// See https://en.wikipedia.org/wiki/Padding_(cryptography)#PKCS7 for padding scheme.
// NOTES: you need to set IV in ctx via AES_init_ctx_iv() or AES_ctx_set_iv()
//        no IV should ever be reused with the same key 
void AES_CBC_encrypt_buffer(AES_ctx_t* ctx, uint8_t* buf, size_t length);
void AES_CBC_decrypt_buffer(AES_ctx_t* ctx, uint8_t* buf, size_t length);

#endif

#endif

