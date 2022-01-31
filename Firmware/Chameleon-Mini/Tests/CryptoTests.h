/* CryptoTests.h */

#ifdef ENABLE_CRYPTO_TESTS

#ifndef __CRYPTO_TESTS_H__
#define __CRYPTO_TESTS_H__

#include "../Common.h"
#include "../Terminal/Terminal.h"
#include "../Application/CryptoTDEA.h"
#include "../Application/CryptoAES128.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

/* DES crypto test cases: */

#ifdef ENABLE_CRYPTO_TDEA_TESTS
/* Test 2KTDEA encryption, ECB mode: */
bool CryptoTDEATestCase1(char *OutParam, uint16_t MaxOutputLength);

/* Test 2KTDEA encryption, CBC receive mode: */
bool CryptoTDEATestCase2(char *OutParam, uint16_t MaxOutputLength);
#endif

#ifdef ENABLE_CRYPTO_3DES_TESTS
/* Test 3DES encryption, CBC for DESFire AuthISO (0x1a): */
bool Crypto3DESTestCase1(char *OutParam, uint16_t MaxOutputLength);
#endif

/* AES-128 crypto test cases: */

#ifdef ENABLE_CRYPTO_AES_TESTS
/* Test AES-128 encrypt/decrypt for a single block (ECB mode): */
bool CryptoAESTestCase1(char *OutParam, uint16_t MaxOutputLength);

/* Test AES-128 encrypt/decrypt for a single-block buffer (CBC mode, with an IV) -- Version 1:
 * Adapted from: https://github.com/eewiki/asf/blob/master/xmega/drivers/aes/example2/aes_example2.c
 */
bool CryptoAESTestCase2(char *OutParam, uint16_t MaxOutputLength);
#endif

#endif /* __CRYPTO_TESTS_H__ */

#endif /* ENABLE_CRYPTO_TESTS */
