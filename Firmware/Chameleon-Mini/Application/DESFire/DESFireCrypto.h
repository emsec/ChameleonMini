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
 * DESFireCrypto.h
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_CRYPTO_H__
#define __DESFIRE_CRYPTO_H__

#include "../../Common.h"

#include "DESFireFirmwareSettings.h"

/* Communication modes:
 * Define the modes of communication over the RFID channel
 *
 * Note there is also an AES wrapped COMMS variant called
 * LRP Secure Messaging detailed starting on page 37
 * (Section 7.2) of
 * https://www.nxp.com/docs/en/application-note/AN12343.pdf
 */
#define DESFIRE_COMMS_PLAINTEXT            (0x00)
#define DESFIRE_COMMS_PLAINTEXT_MAC        (0x01)
#define DESFIRE_COMMS_CIPHERTEXT_DES       (0x03)
#define DESFIRE_COMMS_CIPHERTEXT_AES128    (0x04)
#define DESFIRE_DEFAULT_COMMS_STANDARD     (DESFIRE_COMMS_PLAINTEXT)

#define CRYPTO_TYPE_ANY         (0x00)
#define CRYPTO_TYPE_DES         (0x01)
#define CRYPTO_TYPE_2KTDEA      (0x0A)
#define CRYPTO_TYPE_3K3DES      (0x1A)
#define CRYPTO_TYPE_AES128      (0x4A)

#define CryptoTypeDES(ct) \
    ((ct == CRYPTO_TYPE_DES) || (ct == CRYPTO_TYPE_ANY))
#define CryptoType2KTDEA(ct) \
    ((ct == CRYPTO_TYPE_2KTDEA) || (ct == CRYPTO_TYPE_ANY))
#define CryptoType3KTDEA(ct) \
    ((ct == CRYPTO_TYPE_3K3DES) || (ct == CRYPTO_TYPE_ANY))
#define CryptoTypeAES(ct) \
    ((ct == CRYPTO_TYPE_AES128) || (ct == CRYPTO_TYPE_ANY))

/* Key sizes, block sizes (in bytes): */
#define CRYPTO_AES_KEY_SIZE                  (16)
#define CRYPTO_MAX_KEY_SIZE                  (24)
#define CRYPTO_MAX_BLOCK_SIZE                (16)
#define DESFIRE_AES_IV_SIZE                  (CRYPTO_AES_BLOCK_SIZE)
#define DESFIRE_SESSION_KEY_SIZE             (CRYPTO_3KTDEA_KEY_SIZE)
#define CRYPTO_CHALLENGE_RESPONSE_BYTES      (8)

typedef BYTE CryptoKeyBufferType[CRYPTO_MAX_KEY_SIZE];
typedef BYTE CryptoIVBufferType[CRYPTO_MAX_BLOCK_SIZE];

extern CryptoKeyBufferType SessionKey;
extern CryptoIVBufferType SessionIV;
extern BYTE SessionIVByteSize;

extern uint8_t Authenticated;
extern uint8_t AuthenticatedWithKey;
extern uint8_t AuthenticatedWithPICCMasterKey;
extern uint8_t CryptoAuthMethod;
extern uint8_t ActiveCommMode;

/* Need to invalidate the authentication state after:
 * 1) Selecting a new application;
 * 2) Changing the active key used in the authentication;
 * 3) A failed authentication;
 */
void InvalidateAuthState(BYTE keepPICCAuthData);
bool IsAuthenticated(void);

BYTE GetDefaultCryptoMethodKeySize(uint8_t cryptoType);
BYTE GetCryptoMethodCommSettings(uint8_t cryptoType);
const char *GetCryptoMethodDesc(uint8_t cryptoType);
const char *GetCommSettingsDesc(uint8_t cryptoType);

#define DESFIRE_MAC_LENGTH          4
#define DESFIRE_CMAC_LENGTH         8    // in bytes

/* Authentication status */
#define DESFIRE_NOT_AUTHENTICATED   0xFF

typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
    DESFIRE_AUTH_LEGACY,
    DESFIRE_AUTH_ISO_2KTDEA,
    DESFIRE_AUTH_ISO_3KTDEA,
    DESFIRE_AUTH_AES,
} DesfireAuthType;

BYTE GetCryptoKeyTypeFromAuthenticateMethod(BYTE authCmdMethod);

#define CryptoBitsToBytes(cryptoBits) \
     (cryptoBits / BITS_PER_BYTE)

/*********************************************************
 * AES (128) crypto routines:
 *********************************************************/

#include "../CryptoAES128.h"

typedef uint8_t DesfireAESCryptoKey[CRYPTO_AES_KEY_SIZE];

extern CryptoAESConfig_t AESCryptoContext;
extern DesfireAESCryptoKey AESCryptoSessionKey;
extern DesfireAESCryptoKey AESCryptoIVBuffer;

void InitAESCryptoKeyData(DesfireAESCryptoKey *cryptoKeyData);

typedef void (*CryptoAESCBCFuncType)(uint16_t, void *, void *, uint8_t *, uint8_t *);

typedef uint8_t (*CryptoTransferSendFunc)(uint8_t *, uint8_t);
typedef uint8_t (*CryptoTransferReceiveFunc)(uint8_t *, uint8_t);
uint8_t CryptoAESTransferEncryptSend(uint8_t *Buffer, uint8_t Count, const uint8_t *Key);
uint8_t CryptoAESTransferEncryptReceive(uint8_t *Buffer, uint8_t Count, const uint8_t *Key);

#define DESFIRE_MAX_PAYLOAD_AES_BLOCKS        (DESFIRE_MAX_PAYLOAD_SIZE / CRYPTO_AES_BLOCK_SIZE)

/*********************************************************
 * TripleDES crypto routines:
 *********************************************************/

#include "../CryptoTDEA.h"

#define DESFIRE_2KTDEA_NONCE_SIZE            (CRYPTO_DES_BLOCK_SIZE)
#define DESFIRE_DES_IV_SIZE                  (CRYPTO_DES_BLOCK_SIZE)
#define DESFIRE_MAX_PAYLOAD_TDEA_BLOCKS      (DESFIRE_MAX_PAYLOAD_SIZE / CRYPTO_DES_BLOCK_SIZE)

/* Checksum routines: */
void TransferChecksumUpdateCRCA(const uint8_t *Buffer, uint8_t Count);
uint8_t TransferChecksumFinalCRCA(uint8_t *Buffer);
void TransferChecksumUpdateMACTDEA(const uint8_t *Buffer, uint8_t Count);
uint8_t TransferChecksumFinalMACTDEA(uint8_t *Buffer);

#endif
