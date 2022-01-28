/*
 * CryptoDES.h
 *
 *  Created on: 18.10.2016
 *      Author: dev_zzo
 */

#ifndef CRYPTODES_H_
#define CRYPTODES_H_

#include <stdint.h>
#include "../Common.h"

/* Notes on cryptography in DESFire cards

The EV0 was the first chip in the DESFire series. It makes use of the TDEA
encryption to secure the comms. It also employs CBC to make things more secure.

CBC is used in two "modes": "send mode" and "receive mode".
- Sending data uses the same structure as conventional encryption with CBC
  (XOR with the IV then apply block cipher)
- Receiving data uses the same structure as conventional decryption with CBC
  (apply block cipher then XOR with the IV)

Both operations employ TDEA encryption on the PICC's side and decryption on
PCD's side.

*/

/* Key sizes, in bytes */
#define CRYPTO_DES_KEY_SIZE         8 /* Bytes */
#define CRYPTO_2KTDEA_KEY_SIZE      (CRYPTO_DES_KEY_SIZE * 2)
#define CRYPTO_3KTDEA_KEY_SIZE      (CRYPTO_DES_KEY_SIZE * 3)

typedef uint8_t Crypto2KTDEAKeyType[CRYPTO_2KTDEA_KEY_SIZE];
typedef uint8_t Crypto3KTDEAKeyType[CRYPTO_3KTDEA_KEY_SIZE];

#define CRYPTO_DES_BLOCK_SIZE       8 /* Bytes */
#define CRYPTO_2KTDEA_BLOCK_SIZE             (CRYPTO_DES_BLOCK_SIZE)
#define CRYPTO_3KTDEA_BLOCK_SIZE             (CRYPTO_DES_BLOCK_SIZE)

/* Prototype the CBC function pointer in case anyone needs it */
typedef void (*CryptoTDEACBCFuncType)(uint16_t Count, const void *Plaintext, void *Ciphertext, void *IV, const uint8_t *Keys);
typedef void (*CryptoTDEAFuncType)(const void *PlainText, void *Ciphertext, const uint8_t *Keys);

void CryptoEncryptDEA(void *Plaintext, void *Ciphertext, const uint8_t *Keys);
void CryptoDecryptDEA(void *Plaintext, void *Ciphertext, const uint8_t *Keys);
void EncryptDESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *Keys);
void DecryptDESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *Keys);

/** Performs the Triple DEA enciphering in ECB mode (single block)
 *
 * \param Plaintext     Source buffer with plaintext
 * \param Ciphertext    Destination buffer to contain ciphertext
 * \param Keys          Key block pointer (CRYPTO_2KTDEA_KEY_SIZE)
 */
void CryptoEncrypt2KTDEA(const void *Plaintext, void *Ciphertext, const uint8_t *Keys);
void CryptoDecrypt2KTDEA(const void *Plaintext, void *Ciphertext, const uint8_t *Keys);

void CryptoEncrypt3KTDEA(void *Plaintext, void *Ciphertext, const uint8_t *Keys);
void CryptoDecrypt3KTDEA(void *Plaintext, void *Ciphertext, const uint8_t *Keys);

void Encrypt3DESBuffer(uint16_t Count, const void *Plaintext, void *Ciphertext, const uint8_t *Keys);
void Decrypt3DESBuffer(uint16_t Count, void *Plaintext, const void *Ciphertext, const uint8_t *Keys);


/** Performs the 2-key Triple DES en/deciphering in the CBC "send" mode (xor-then-crypt)
 *
 * \param Count         Block count, expected to be >= 1
 * \param Plaintext     Source buffer with plaintext
 * \param Ciphertext    Destination buffer to contain ciphertext
 * \param IV            Initialization vector buffer, will be updated
 * \param Keys          Key block pointer (CRYPTO_2KTDEA_KEY_SIZE)
 */
void CryptoEncrypt2KTDEA_CBCSend(uint16_t Count, const void *Input, void *Output, void *IV, const uint8_t *Keys);
void CryptoDecrypt2KTDEA_CBCSend(uint16_t Count, const void *Input, void *Output, void *IV, const uint8_t *Keys);

/** Performs the 2-key Triple DES en/deciphering in the CBC "receive" mode (crypt-then-xor)
 *
 * \param Count         Block count, expected to be >= 1
 * \param Plaintext     Source buffer with plaintext
 * \param Ciphertext    Destination buffer to contain ciphertext
 * \param IV            Initialization vector buffer, will be updated
 * \param Keys          Key block pointer (CRYPTO_2KTDEA_KEY_SIZE)
 */
void CryptoEncrypt2KTDEA_CBCReceive(uint16_t Count, const void *Input, void *Output, void *IV, const uint8_t *Keys);
void CryptoDecrypt2KTDEA_CBCReceive(uint16_t Count, const void *Input, void *Output, void *IV, const uint8_t *Keys);


/** Performs the 3-key Triple DES en/deciphering in the CBC "send" or "receive" mode
 *
 * \param Count         Block count, expected to be >= 1
 * \param Plaintext     Source buffer with plaintext
 * \param Ciphertext    Destination buffer to contain ciphertext
 * \param IV            Initialization vector buffer, will be updated
 * \param Keys          Key block pointer (CRYPTO_3KTDEA_KEY_SIZE)
 */
void CryptoEncrypt3KTDEA_CBCSend(uint16_t Count, const void *Plaintext, void *Ciphertext, void *IV, const uint8_t *Keys);
void CryptoDecrypt3KTDEA_CBCReceive(uint16_t Count, const void *Plaintext, void *Ciphertext, void *IV, const uint8_t *Keys);

/* Spec for more generic send/recv encrypt/decrypt schemes: */
typedef struct {
    CryptoTDEAFuncType cryptFunc;
    uint16_t           blockSize;
} CryptoTDEA_CBCSpec;

void CryptoTDEA_CBCSend(uint16_t Count, void *Plaintext, void *Ciphertext,
                        void *IV, const uint8_t *Keys, CryptoTDEA_CBCSpec CryptoSpec);
void CryptoTDEA_CBCRecv(uint16_t Count, void *Plaintext, void *Ciphertext,
                        void *IV, const uint8_t *Keys, CryptoTDEA_CBCSpec CryptoSpec);

uint8_t TransferEncryptTDEASend(uint8_t *Buffer, uint8_t Count);
uint8_t TransferEncryptTDEAReceive(uint8_t *Buffer, uint8_t Count);

/** Applies padding to the data within the buffer
 *
 * \param Buffer 		Data buffer to pad
 * \param BytesInBuffer	How much data there is in the buffer already
 * \param FirstPaddingBitSet Whether the very first bit (MSB) will be set or not
 */
INLINE void CryptoPaddingTDEA(uint8_t *Buffer, uint8_t BytesInBuffer, bool FirstPaddingBitSet) {
    uint8_t PaddingByte = FirstPaddingBitSet << 7;
    uint8_t i;

    for (i = BytesInBuffer; i < CRYPTO_DES_BLOCK_SIZE; ++i) {
        Buffer[i] = PaddingByte;
        PaddingByte = 0x00;
    }
}

#endif /* CRYPTODES_H_ */
