#ifndef CRYPTO1_H
#define CRYPTO1_H

#include <stdint.h>
#include <stdbool.h>

void Crypto1GetState(uint8_t* pEven, uint8_t* pOdd);

/* Gets the current keystream-bit, without shifting the internal LFSR */
uint8_t Crypto1FilterOutput(void);

/* Set up Crypto1 cipher using the given Key, Uid and CardNonce. Also encrypts
 * the CardNonce in-place while in non-linear mode. */
void Crypto1Setup(uint8_t Key[6], uint8_t Uid[4], uint8_t CardNonce[4]);
/* Same for nested auth. CardNonce[4]..[7] will contain the parity bits after return */
void Crypto1SetupNested(uint8_t Key[6], uint8_t Uid[4], uint8_t CardNonce[8], bool Decrypt);

/* Load the decrypted ReaderNonce into the Crypto1 state LFSR */
void Crypto1Auth(uint8_t EncryptedReaderNonce[4]);

/* Generate 8 Bits of key stream */
uint8_t Crypto1Byte(void);

/* Encrypt/Decrypt array */
void Crypto1ByteArray(uint8_t* Buffer, uint8_t Count);
void Crypto1ByteArrayWithParity(uint8_t* Buffer, uint8_t Count);

/* Generate 4 Bits of key stream */
uint8_t Crypto1Nibble(void);

/* Execute 'ClockCount' cycles on the PRNG state 'State' */
void Crypto1PRNG(uint8_t State[4], uint8_t ClockCount);

/* Encrypts buffer with consideration of parity bits */
void Crypto1EncryptWithParity(uint8_t * Buffer, uint8_t BitCount);

/* Encrypts buffer with LFSR feedback within reader nonce and considers parity bits */
void Crypto1ReaderAuthWithParity(uint8_t PlainReaderAnswerWithParityBits[9]);

#endif //CRYPTO1_H
