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
 * Copyright (C) 2012 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* aes128.h : Standalone C library adapted from the ArduinoCryptoLib source to 
 *            implement AES128 encryption with a small foorprint. 
 */

#ifndef __AES128_CRYPTO_H__
#define __AES128_CRYPTO_H__

#define AES128_CRYPTO_ROUNDS              (10)
#define AES128_CRYPTO_SCHEDULE_SIZE       (16)
#define AES128_BLOCK_SIZE                 (16)
#define AES128_KEY_SIZE                   (16)

typedef struct {
    uint8_t     rounds;
    uint8_t     schedule[AES128_CRYPTO_SCHEDULE_SIZE];
    uint8_t     reverse[AES128_CRYPTO_SCHEDULE_SIZE];
    uint8_t     sched[176];
    uint8_t     keyData[AES128_KEY_SIZE];
} AES128Context;

void aes128InitContext(AES128Context *ctx);
void aes128ClearContext(AES128Context *ctx);
bool aes128SetKey(AES128Context *ctx, const uint8_t *keyData, size_t keySize);
void aes128EncryptBlock(AES128Context *ctx, uint8_t *ctBlockBuf, const uint8_t *ptBlockBuf);
void aes128DecryptBlock(AES128Context *ctx, uint8_t *ptBlockBuf, const uint8_t *ctBlockBuf);

#endif
