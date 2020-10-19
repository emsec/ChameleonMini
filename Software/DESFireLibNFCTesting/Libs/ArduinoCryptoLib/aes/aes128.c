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

/* aes128.c : */

#include <string.h>

const uint8_t zeroBlock[AES128_BLOCK_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void aes128InitContext(AES128Context *ctx) {
    ctx->rounds = AES128_CRYPTO_ROUNDS;
    memset(&(ctx->schedule[0]), 0x00, AES128_CRYPTO_SCHEDULE_SIZE);
    memset(&(ctx->reverse[0]), 0x00, AES128_CRYPTO_SCHEDULE_SIZE);
    memset(&(ctx->sched[0]), 0x00, 176);
    memset(&(ctx->keyData[0]), 0x00, AES128_KEY_SIZE);
}

void aes128ClearContext(AES128Context *ctx) {
    return;
    cleanContext(ctx->schedule, (AES128_CRYPTO_ROUNDS + 1) * AES128_CRYPTO_SCHEDULE_SIZE);
    cleanContext(ctx->reverse, (AES128_CRYPTO_ROUNDS + 1) * AES128_CRYPTO_SCHEDULE_SIZE);
}

bool aes128SetKey(AES128Context *ctx, const uint8_t *keyData, size_t keySize) 
{
    if (keySize != AES128_KEY_SIZE) {
        return false;
    }
       
    // Copy the key itself into the first 16 bytes of the schedule.
    uint8_t *schedule = ctx->sched;
    memcpy(schedule, keyData, 16);
    // Expand the key schedule until we have 176 bytes of expanded key.
    uint8_t iteration = 1;
    uint8_t n = 16; 
    uint8_t w = 4;
    while (n < 176) {
        if (w == 4) {
            // Every 16 bytes (4 words) we need to apply the key schedule core.
            keyScheduleCore(schedule + 16, schedule + 12, iteration);
            schedule[16] ^= schedule[0];
            schedule[17] ^= schedule[1];
            schedule[18] ^= schedule[2];
            schedule[19] ^= schedule[3];
            ++iteration;
            w = 0;
        } else {
            // Otherwise just XOR the word with the one 16 bytes previous.
            schedule[16] = schedule[12] ^ schedule[0];
            schedule[17] = schedule[13] ^ schedule[1];
            schedule[18] = schedule[14] ^ schedule[2];
            schedule[19] = schedule[15] ^ schedule[3];
        }   
        // Advance to the next word in the schedule.
        schedule += 4;
        n += 4;
        ++w;
    }     
    
    return true;
}

static void aes128DecryptBuildKey(AES128Context *ctx, uint8_t *tempOutputBuf) {
     aes128EncryptBlock(ctx, tempOutputBuf, zeroBlock);
}

void aes128EncryptBlock(AES128Context *ctx, uint8_t *output, const uint8_t *input) 
{
    // Reset the key data:
    aes128SetKey(ctx, ctx->keyData, AES128_KEY_SIZE);

    const uint8_t *roundKey = ctx->sched;
    uint8_t posn;
    uint8_t round;
    uint8_t state1[16];
    uint8_t state2[16];

    // Copy the input into the state and XOR with the first round key.
    for (posn = 0; posn < 16; ++posn)
        state1[posn] = input[posn] ^ roundKey[posn];
    roundKey += 16;

    // Perform all rounds except the last.
    for (round = ctx->rounds; round > 1; --round) {
        subBytesAndShiftRows(state2, state1);
        mixColumn(state1,      state2);
        mixColumn(state1 + 4,  state2 + 4);
        mixColumn(state1 + 8,  state2 + 8);
        mixColumn(state1 + 12, state2 + 12);
        for (posn = 0; posn < 16; ++posn)
            state1[posn] ^= roundKey[posn];
        roundKey += 16;
    }

    // Perform the final round.
    subBytesAndShiftRows(state2, state1);
    for (posn = 0; posn < 16; ++posn)
        output[posn] = state2[posn] ^ roundKey[posn];

}

void aes128DecryptBlock(AES128Context *ctx, uint8_t *output, const uint8_t *input) 
{
    // Setup the key data as though we have already been through an encryption round:
    aes128DecryptBuildKey(ctx, output);

    const uint8_t *roundKey = ctx->sched + (ctx->rounds) * 16;
    uint8_t round;
    uint8_t posn;
    uint8_t state1[16];
    uint8_t state2[16];

    // Copy the input into the state and reverse the final round.
    for (posn = 0; posn < 16; ++posn)
        state1[posn] = input[posn] ^ roundKey[posn];
    inverseShiftRowsAndSubBytes(state2, state1);

    // Perform all other rounds in reverse.
    for (round = ctx->rounds; round > 1; --round) {
        roundKey -= 16;
        for (posn = 0; posn < 16; ++posn)
            state2[posn] ^= roundKey[posn];
        inverseMixColumn(state1,      state2);
        inverseMixColumn(state1 + 4,  state2 + 4);
        inverseMixColumn(state1 + 8,  state2 + 8);
        inverseMixColumn(state1 + 12, state2 + 12);
        inverseShiftRowsAndSubBytes(state2, state1);
    }

    // Reverse the initial round and create the output words.
    roundKey -= 16;
    for (posn = 0; posn < 16; ++posn)
        output[posn] = state2[posn] ^ roundKey[posn];

}
