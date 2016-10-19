/*
 * CryptoDES.h
 *
 *  Created on: 18.10.2016
 *      Author: dev_zzo
 */

#ifndef CRYPTODES_H_
#define CRYPTODES_H_

#include <stdint.h>

#define CRYPTO_DES_KEY_SIZE 8

/** Performs the Triple DES sequence in the CBC mode on the buffer */
void CryptoEncrypt_3DES_KeyOption2_CBC(const uint8_t* Keys, void* Buffer, uint16_t Count);

#endif /* CRYPTODES_H_ */
