/*
 * Copyright (C) 2015 Southern Storm Software, Pty Ltd.
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

/* aes-common.h : Common utilities for AES crypto. */

#ifndef __AES128_CRYPTO_COMMON_UTILS_H__
#define __AES128_CRYPTO_COMMON_UTILS_H__

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>

#define KCORE(n) \
    do { \
        keyScheduleCore(temp, schedule + 12, (n)); \
        schedule[0] ^= temp[0]; \
        schedule[1] ^= temp[1]; \
        schedule[2] ^= temp[2]; \
        schedule[3] ^= temp[3]; \
    } while (0)

#define KXOR(a, b) \
    do { \
        schedule[(a) * 4] ^= schedule[(b) * 4]; \
        schedule[(a) * 4 + 1] ^= schedule[(b) * 4 + 1]; \
        schedule[(a) * 4 + 2] ^= schedule[(b) * 4 + 2]; \
        schedule[(a) * 4 + 3] ^= schedule[(b) * 4 + 3]; \
    } while (0)

void cleanContext(void *dest, size_t size);
bool secure_compare(const void *data1, const void *data2, size_t len);

// Multiply x by 2 in the Galois field, to achieve the effect of the following:
//
//     if (x & 0x80)
//         return (x << 1) ^ 0x1B;
//     else
//         return (x << 1);
//
// However, we don't want to use runtime conditionals if we can help it
// to avoid leaking timing information from the implementation.
// In this case, multiplication is slightly faster than table lookup on AVR.
#define gmul2(x)    (t = ((uint16_t)(x)) << 1, \
                     ((uint8_t)t) ^ (uint8_t)(0x1B * ((uint8_t)(t >> 8))))

// Multiply x by 4 in the Galois field.
#define gmul4(x)    (t = ((uint16_t)(x)) << 2, ((uint8_t)t) ^ K[t >> 8])

// Multiply x by 8 in the Galois field.
#define gmul8(x)    (t = ((uint16_t)(x)) << 3, ((uint8_t)t) ^ K[t >> 8])

#define OUT(col, row)   output[(col) * 4 + (row)]
#define IN(col, row)    input[(col) * 4 + (row)]

void subBytesAndShiftRows(uint8_t *output, const uint8_t *input);
void inverseShiftRowsAndSubBytes(uint8_t *output, const uint8_t *input);
void mixColumn(uint8_t *output, uint8_t *input);
void inverseMixColumn(uint8_t *output, const uint8_t *input);
void keyScheduleCore(uint8_t *output, const uint8_t *input, uint8_t iteration);
void applySbox(uint8_t *output, const uint8_t *input);

#endif
