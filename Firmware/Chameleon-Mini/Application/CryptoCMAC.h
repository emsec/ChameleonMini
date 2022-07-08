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
 * CryptoCMAC.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __CRYPTO_CMAC_H__
#define __CRYPTO_CMAC_H__

#include "DESFire/DESFireCrypto.h"
#include "CryptoTDEA.h"
#include "CryptoAES128.h"

/* CMAC and MAC source code based on @github/andrade/nfcjlib */

#define CRYPTO_CMAC_RB64           (0x1B)
#define CRYPTO_CMAC_RB128          ((uint8_t) 0x87)

bool appendBufferCMAC(uint8_t cryptoType, const uint8_t *keyData, uint8_t *bufferData, uint16_t bufferSize, uint8_t *IV);
bool checkBufferMAC(uint8_t *bufferData, uint16_t bufferSize, uint16_t checksumSize);

uint16_t appendBufferMAC(const uint8_t *keyData, uint8_t *bufferData, uint16_t bufferSize);
bool checkBufferCMAC(uint8_t *bufferData, uint16_t bufferSize, uint16_t checksumSize);

#endif
