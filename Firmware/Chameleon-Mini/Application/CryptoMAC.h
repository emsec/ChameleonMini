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
 * CryptoMAC.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __CRYPTO_MAC_H__
#define __CRYPTO_MAC_H__

#include "DESFire/DESFireCrypto.h"
#include "CryptoTDEA.h"
#include "CryptoAES128.h"

/* MAC and CMAC source code based on github/andrade/nfcjlib */

#define CRYPTO_CMAC_RB64           (0x1B)
#define CRYPTO_CMAC_RB128          ((uint8_t) 0x87)

void getCMACSubK1(uint8_t *bufferL, uint16_t blockSize, uint8_t polyByte, uint8_t *bufferOut);
void getCMACSubK2(uint8_t *bufferK1, uint16_t blockSize, uint8_t polyByte, uint8_t *bufferOut);

bool computeBufferCMACFull(uint8_t *keyData, uint8_t *bufferK1, uint8_t *bufferK2, uint8_t *bufferIV, uint16_t blockSize, uint16_t cryptoType);
bool computeBufferCMAC(uint16_t cryptoType, uint8_t *keyData, uint8_t *bufferData, uint8_t *aesIV);

bool computeMac(uint8_t *bufferData, uint16_t dataLength, uint8_t *keyData, uint16_t cryptoKeyType);

#endif
