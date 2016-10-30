/*
 * CryptoDES.h
 *
 *  Created on: 18.10.2016
 *      Author: dev_zzo
 */

#ifndef CRYPTODES_H_
#define CRYPTODES_H_

#include <stdint.h>

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

#define CRYPTO_DES_BLOCK_SIZE       8 /* Bytes */

/** Performs the Triple DES sequence in the CBC mode on the buffer */
void CryptoEncrypt_2KTDEA_CBC_Send(uint16_t Count, const void* Plaintext, void* Ciphertext, void *IV, const uint8_t* Keys);
void CryptoEncrypt_2KTDEA_CBC_Receive(uint16_t Count, const void* Plaintext, void* Ciphertext, void *IV, const uint8_t* Keys);

#endif /* CRYPTODES_H_ */
