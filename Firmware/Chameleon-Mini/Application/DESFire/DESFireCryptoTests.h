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
*/

/* DESFireCryptoTests.h : Since we cannot easily compile the crypto sources as a standalone unit, 
 *                        this mechanism will allow to print some unit tests to the live logging 
 *                        console when a new DESFire emulation instance is initiated. At a minimum, 
 *                        this should help to verify that any side effects and errors in the 
 *                        protocols are not based on this logical part of the scheme. 
 */

#ifndef __DESFIRE_CRYPTO_TESTS_H__
#define __DESFIRE_CRYPTO_TESTS_H__

#ifdef DESFIRE_RUN_CRYPTO_TESTING_PROCEDURE

#include "../../Common.h"

#include "DESFireFirmwareSettings.h"
#include "DESFireMemoryOperations.h"
#include "DESFireLogging.h"
#include "DESFireCrypto.h"

typedef bool (*UnitTestResultFunc)(uint8_t *errorResultBuf, uint8_t *bufSize);

INLINE bool DiffCryptoResult(const uint8_t *bufData, const uint8_t *cmpBuf, uint8_t bufSize);
INLINE bool RunUnitTest(UnitTestResultFunc unitTestRunnerFunc);
INLINE bool RunCryptoUnitTests(void);

INLINE bool TestDesfire2KTDEA(uint8_t *errorResultBuf, uint8_t *bufSize);
INLINE bool TestDesfire3K3DES(uint8_t *errorResultBuf, uint8_t *bufSize);
INLINE bool TestDesfireAES128(uint8_t *errorResultBuf, uint8_t *bufSize);

INLINE bool 
DiffCryptoResult(const uint8_t *bufData, const uint8_t *cmpBuf, uint8_t bufSize) {
    return !memcmp(bufData, cmpBuf, bufSize);
}

INLINE bool 
RunUnitTest(UnitTestResultFunc unitTestRunnerFunc) {
    uint16_t printResultLogSize = 0x00; 
    uint8_t errorResultBuf[CRYPTO_MAX_BLOCK_SIZE];
    uint8_t errorBufSize = CRYPTO_MAX_BLOCK_SIZE;
    bool unitTestPassed = unitTestRunnerFunc(errorResultBuf, &errorBufSize);
    return unitTestPassed;
}

INLINE bool
RunCryptoUnitTests(void) {
     UnitTestResultFunc unitTestRunnerFuncs[] = {
         //&TestDesfire2KTDEA, 
         //&TestDesfire3K3DES, 
         //&TestDesfireAES128, 
     };
     uint8_t numUnitTests = sizeof(unitTestRunnerFuncs) / sizeof(UnitTestResultFunc);
     uint8_t utIndex = 0x00;
     while(utIndex < numUnitTests) {
          if(!RunUnitTest(unitTestRunnerFuncs[utIndex])) {
              uint8_t resultLogSize = 0x00;
              return false;
          }
          utIndex++;
     }
     uint8_t resultLogSize = 0x00;
     snprintf_P(__InternalStringBuffer, STRING_BUFFER_SIZE, PSTR("All crypto unit tests passed!"));
     resultLogSize = StringLength(__InternalStringBuffer, STRING_BUFFER_SIZE);
     DesfireLogEntry(LOG_INFO_DESFIRE_DEBUGGING_OUTPUT, (void *) __InternalStringBuffer, resultLogSize);
     return true;
}

#if 0
INLINE bool TestDesfire2KTDEA(uint8_t *errorResultBuf, uint8_t *bufSize) {

     uint8_t cryptoDataByteCount = 2 * CRYPTO_DES_BLOCK_SIZE;
     uint8_t keyData[CRYPTO_2KTDEA_KEY_SIZE] = { 
         0xB4, 0x28, 0x2E, 0xFA, 0x9E, 0xB8, 0x2C, 0xAE, 
         0xB4, 0x28, 0x2E, 0xFA, 0x9E, 0xB8, 0x2C, 0xAE
     };
     uint8_t cryptoGramData[] = {
         0x00, 0x10, 0x20, 0x31, 0x40, 0x50, 0x60, 0x70, 
         0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80, 
         //0xC5, 0xFF, 0x01, 0x50, 0x00, 0x00, 0x00, 0x00
     };
     uint8_t cryptoGramEncData[] = {
         0x87, 0x99, 0x59, 0x11, 0x8B, 0xD7, 0x7C, 0x70, 
         0x10, 0x7B, 0xCD, 0xB0, 0xC0, 0x9C, 0xC7, 0xDA, 
         //0x82, 0x15, 0x04, 0xAA, 0x1E, 0x36, 0x04, 0x9C
     };
     uint8_t cryptoResult[cryptoDataByteCount];
     uint8_t ivBuf[CRYPTO_2KTDEA_BLOCK_SIZE];
     
     memset(ivBuf, 0x00, CRYPTO_2KTDEA_BLOCK_SIZE);
     CryptoEncrypt2KTDEA_CBCSend(cryptoDataByteCount, cryptoGramData, cryptoResult, ivBuf, keyData);
     if(!DiffCryptoResult(cryptoGramEncData, cryptoResult, cryptoDataByteCount)) {
          BufferToHexString(__InternalStringBuffer, STRING_BUFFER_SIZE, cryptoResult, cryptoDataByteCount);
          DesfireLogEntry(LOG_INFO_DESFIRE_DEBUGGING_OUTPUT, (void *) __InternalStringBuffer, 2 * cryptoDataByteCount);
          return false;
     }
     return true;

}
#endif

#if 0
INLINE bool TestDesfire3K3DES(uint8_t *errorResultBuf, uint8_t *bufSize) {

     const uint8_t cryptoDataByteCount = 2 * CRYPTO_3KTDEA_BLOCK_SIZE;
     const uint8_t keyData[CRYPTO_3KTDEA_KEY_SIZE] = { 
         0xF4, 0x68, 0x6E, 0x3A, 0xBA, 0x90, 0x36, 0xBA, 
         0xD2, 0x8E, 0xBC, 0x10, 0x32, 0xE6, 0x38, 0xF0, 
         0x80, 0x44, 0x5A, 0xF6, 0x06, 0x86, 0xD0, 0xC4
     };
     const uint8_t cryptoGramData[] = {
         0x00, 0x10, 0x20, 0x31, 0x40, 0x50, 0x60, 0x70, 
         0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80, 
         //0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 
         //0xD6, 0x3E, 0x00, 0xA2, 0x00, 0x00, 0x00, 0x00
     };
     const uint8_t cryptoGramEncData[] = {
         0x7F, 0x88, 0x90, 0xC7, 0xCA, 0xB9, 0xA4, 0x22, 
         0x81, 0x73, 0xA6, 0x41, 0xB6, 0x5F, 0x0F, 0x43, 
         //0xFD, 0x40, 0x4A, 0x01, 0x13, 0x71, 0xA9, 0x90, 
         //0x4A, 0x62, 0x9E, 0x3C, 0x20, 0xB2, 0xFF, 0x63
     };
     uint8_t cryptoResult[cryptoDataByteCount];
     uint8_t ivBuf[CRYPTO_3KTDEA_BLOCK_SIZE];
     
     memset(ivBuf, 0x00, CRYPTO_3KTDEA_BLOCK_SIZE);
     CryptoEncrypt3KTDEA_CBCSend(cryptoDataByteCount, cryptoGramData, cryptoResult, ivBuf, keyData);
     if(!DiffCryptoResult(cryptoGramEncData, cryptoResult, cryptoDataByteCount)) {
          BufferToHexString(__InternalStringBuffer, STRING_BUFFER_SIZE, cryptoResult, cryptoDataByteCount);
          DesfireLogEntry(LOG_INFO_DESFIRE_DEBUGGING_OUTPUT, (void *) __InternalStringBuffer, 2 * cryptoDataByteCount);
          return false;
     }
     return true;

}
#endif

#if 0
INLINE bool TestDesfireAES128(uint8_t *errorResultBuf, uint8_t *bufSize) {

     const uint8_t cryptoDataByteCount = 2 * CRYPTO_AES_BLOCK_SIZE;
     const uint8_t keyData[CRYPTO_AES_KEY_SIZE] = { 
         0x73, 0xAE, 0x5D, 0x30, 0x1F, 0x45, 0x19, 0x27, 
         0x1F, 0x2A, 0x69, 0x8C, 0xEF, 0x69, 0x76, 0x04
     };
     const uint8_t cryptoGramData[] = {
         0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 
         0x80, 0x90, 0xA0, 0xB0, 0xB0, 0xA0, 0x90, 0x80, 
         0x10, 0xD2, 0xC6, 0xE6, 0x6B, 0x00, 0x00, 0x00, 
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
     };
     const uint8_t cryptoGramEncData[] = {
         0x97, 0x41, 0x8E, 0x6C, 0xC0, 0x1C, 0x4E, 0x6F, 
         0xAD, 0x4D, 0x87, 0x4D, 0x8D, 0x42, 0x5C, 0xEA, 
         0x32, 0x51, 0x36, 0x11, 0x47, 0x2C, 0xDA, 0x04, 
         0xE3, 0x5E, 0xFB, 0x77, 0x9A, 0x7D, 0xA0, 0xE4
     };
     uint8_t cryptoResult[cryptoDataByteCount];
     uint8_t ivBuf[CRYPTO_AES_BLOCK_SIZE];
     
     AESCryptoKeySizeBytes = 16;
     DesfireAESCryptoInit(keyData, AESCryptoKeySizeBytes, &AESCryptoContext);
     memset(ivBuf, 0x00, CRYPTO_AES_BLOCK_SIZE);
     CryptoEncryptAES_CBCSend(cryptoDataByteCount, cryptoGramData, cryptoResult, ivBuf, &AESCryptoContext);
     if(!DiffCryptoResult(cryptoGramEncData, cryptoResult, cryptoDataByteCount)) {
          BufferToHexString(__InternalStringBuffer, STRING_BUFFER_SIZE, cryptoResult, cryptoDataByteCount);
          DesfireLogEntry(LOG_INFO_DESFIRE_DEBUGGING_OUTPUT, (void *) __InternalStringBuffer, 2 * cryptoDataByteCount);
          return false;
     }
     return true;

}
#endif

#endif

#endif
