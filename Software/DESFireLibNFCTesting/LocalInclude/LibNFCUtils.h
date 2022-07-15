/*-
 * Free/Libre Near Field Communication (NFC) library
 *
 * Libnfc historical contributors:
 * Copyright (C) 2009      Roel Verdult
 * Copyright (C) 2009-2013 Romuald Conty
 * Copyright (C) 2010-2012 Romain Tartière
 * Copyright (C) 2010-2013 Philippe Teuwen
 * Copyright (C) 2012-2013 Ludovic Rousseau
 * See AUTHORS file for a more comprehensive list of contributors.
 * Additional contributors of this file:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1) Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  2 )Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Note that this license only applies on the examples, NFC library itself is under LGPL
 *
 */

/**
 * @file nfc-utils.h
 * @brief Provide some examples shared functions like print, parity calculation, options parsing.
 */
/**
 * @file nfc-utils.c
 * @brief Provide some examples shared functions like print, parity calculation, options parsing.
 */

#ifndef _EXAMPLES_NFC_UTILS_H_
#define _EXAMPLES_NFC_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <nfc/nfc.h>

#include "GeneralUtils.h"

/**
 * @macro DBG
 * @brief Print a message of standard output only in DEBUG mode
 */
#ifdef DEBUG
#  define DBG(...) do { \
    warnx ("DBG %s:%d", __FILE__, __LINE__); \
    warnx ("    " __VA_ARGS__ ); \
  } while (0)
#else
#  define DBG(...) {}
#endif

/**
 * @macro WARN
 * @brief Print a warn message
 */
#ifdef DEBUG
#  define WARN(...) do { \
    warnx ("WARNING %s:%d", __FILE__, __LINE__); \
    warnx ("    " __VA_ARGS__ ); \
  } while (0)
#else
#  define WARN(...) warnx ("WARNING: " __VA_ARGS__ )
#endif

/**
 * @macro ERR
 * @brief Print a error message
 */
#ifdef DEBUG
#  define ERR(...) do { \
    warnx ("ERROR %s:%d", __FILE__, __LINE__); \
    warnx ("    " __VA_ARGS__ ); \
  } while (0)
#else
#  define ERR(...)  warnx ("ERROR: " __VA_ARGS__ )
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

static inline uint8_t oddparity(const uint8_t bt) {
    // cf http://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    return (0x9669 >> ((bt ^ (bt >> 4)) & 0xF)) & 1;
}

static inline void oddparity_bytes_ts(const uint8_t *pbtData, const size_t szLen, uint8_t *pbtPar) {
    size_t  szByteNr;
    // Calculate the parity bits for the command
    for (szByteNr = 0; szByteNr < szLen; szByteNr++) {
        pbtPar[szByteNr] = oddparity(pbtData[szByteNr]);
    }
}

static inline void print_hex(const uint8_t *pbtData, const size_t szLen) {
    size_t  szPos;
    for (szPos = 0; szPos < szLen; szPos++) {
        if ((szPos > 0) && (szPos % 8) == 0) {
            printf("| ");
        }
        printf("%02x ", pbtData[szPos]);
    }
    printf("\n");
}

static inline void print_hex_bits(const uint8_t *pbtData, const size_t szBits) {
    uint8_t uRemainder;
    size_t  szPos;
    size_t  szBytes = szBits / 8;
    for (szPos = 0; szPos < szBytes; szPos++) {
        printf("%02x  ", pbtData[szPos]);
    }
    uRemainder = szBits % 8;
    // Print the rest bits
    if (uRemainder != 0) {
        if (uRemainder < 5)
            printf("%01x (%d bits)", pbtData[szBytes], uRemainder);
        else
            printf("%02x (%d bits)", pbtData[szBytes], uRemainder);
    }
    printf("\n");
}

static inline void print_hex_par(const uint8_t *pbtData, const size_t szBits, const uint8_t *pbtDataPar) {
    uint8_t uRemainder;
    size_t  szPos;
    size_t  szBytes = szBits / 8;
    for (szPos = 0; szPos < szBytes; szPos++) {
        printf("%02x", pbtData[szPos]);
        if (oddparity(pbtData[szPos]) != pbtDataPar[szPos]) {
            printf("! ");
        } else {
            printf("  ");
        }
    }
    uRemainder = szBits % 8;
    // Print the rest bits, these cannot have parity bit
    if (uRemainder != 0) {
        if (uRemainder < 5)
            printf("%01x (%d bits)", pbtData[szBytes], uRemainder);
        else
            printf("%02x (%d bits)", pbtData[szBytes], uRemainder);
    }
    printf("\n");
}

static inline void print_nfc_target(const nfc_target *pnt, bool verbose) {
    char *s;
    str_nfc_target(&s, pnt, verbose);
    printf("%s", s);
    nfc_free(s);
}

typedef struct {
    volatile size_t  recvSzRx;
    volatile uint8_t *rxDataBuf;
    volatile size_t  maxRxDataSize;
} RxData_t;

static inline RxData_t *InitRxDataStruct(size_t bufSize) {
    RxData_t *rxData = malloc(sizeof(RxData_t));
    if (rxData == NULL) {
        return NULL;
    }
    rxData->recvSzRx = 0;
    rxData->rxDataBuf = malloc(bufSize);
    memset(rxData->rxDataBuf, 0x00, bufSize);
    if (rxData->rxDataBuf == NULL) {
        free(rxData);
        return NULL;
    }
    rxData->maxRxDataSize = bufSize;
}

static inline void FreeRxDataStruct(RxData_t *rxData, bool freeInputPtr) {
    if (rxData != NULL) {
        free(rxData->rxDataBuf);
        if (freeInputPtr) {
            free(rxData);
        } else {
            rxData->recvSzRx = 0;
            rxData->maxRxDataSize = 0;
            rxData->rxDataBuf = NULL;
        }
    }
}

static inline bool
libnfcTransmitBits(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTxBytes, RxData_t *rxData) {
    int res;
    if ((res = nfc_initiator_transceive_bits(pnd, pbtTx, ASBITS(szTxBytes), NULL, rxData->rxDataBuf, ASBITS(rxData->maxRxDataSize), NULL)) < 0) {
        fprintf(stderr, "    -- Error transceiving Bits: %s\n", nfc_strerror(pnd));
        return false;
    }
    rxData->recvSzRx = res;
    return true;
}

static inline bool
libnfcTransmitBytes(nfc_device *pnd, const uint8_t *pbtTx, const size_t szTx, RxData_t *rxData) {
    int res;
    if ((res = nfc_initiator_transceive_bytes(pnd, pbtTx, szTx, rxData->rxDataBuf, rxData->maxRxDataSize, 0)) < 0) {
        fprintf(stderr, "    -- Error transceiving Bytes: %s\n", nfc_strerror(pnd));
        return false;
    }
    rxData->recvSzRx = res;
    return true;
}

#endif
