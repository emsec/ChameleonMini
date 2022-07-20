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
 * DESFireCrypto.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include <string.h>

#include "../../Log.h"

#include "../MifareDESFire.h"
#include "DESFireCrypto.h"
#include "DESFireInstructions.h"
#include "DESFirePICCControl.h"
#include "DESFireISO14443Support.h"
#include "DESFireStatusCodes.h"
#include "DESFireLogging.h"

CryptoKeyBufferType SessionKey = { 0 };
CryptoIVBufferType SessionIV = { 0 };
BYTE SessionIVByteSize = 0;
BYTE DesfireCommMode = DESFIRE_DEFAULT_COMMS_STANDARD;

uint16_t AESCryptoKeySizeBytes = 0;
CryptoAESConfig_t AESCryptoContext = { 0 };

bool Authenticated = false;
uint8_t AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
bool AuthenticatedWithPICCMasterKey = false;
uint8_t CryptoAuthMethod = CRYPTO_TYPE_ANY;
uint8_t ActiveCommMode = DESFIRE_DEFAULT_COMMS_STANDARD;

void InvalidateAuthState(BYTE keepPICCAuthData) {
    if (!keepPICCAuthData) {
        AuthenticatedWithPICCMasterKey = false;
        memset(&SessionKey[0], 0x00, CRYPTO_MAX_BLOCK_SIZE);
        memset(&SessionIV[0], 0x00, CRYPTO_MAX_BLOCK_SIZE);
        SessionIVByteSize = 0;
    }
    Authenticated = false;
    AuthenticatedWithKey = DESFIRE_NOT_AUTHENTICATED;
    Iso7816FileSelected = false;
    CryptoAuthMethod = CRYPTO_TYPE_ANY;
}

bool IsAuthenticated(void) {
    return Authenticated != 0;
}

BYTE GetDefaultCryptoMethodKeySize(uint8_t cryptoType) {
    switch (cryptoType) {
        case CRYPTO_TYPE_ANY:
            return CRYPTO_3KTDEA_KEY_SIZE;
        case CRYPTO_TYPE_2KTDEA:
            return CRYPTO_2KTDEA_KEY_SIZE;
        case CRYPTO_TYPE_3K3DES:
            return CRYPTO_3KTDEA_KEY_SIZE;
        case CRYPTO_TYPE_AES128:
            return CRYPTO_AES_BLOCK_SIZE;
        case CRYPTO_TYPE_DES:
        default:
            return CRYPTO_DES_BLOCK_SIZE;
    }
}

BYTE GetCryptoMethodCommSettings(uint8_t cryptoType) {
    switch (cryptoType) {
        case CRYPTO_TYPE_2KTDEA:
            return DESFIRE_COMMS_PLAINTEXT_MAC;
        case CRYPTO_TYPE_3K3DES:
            return DESFIRE_COMMS_CIPHERTEXT_DES;
        case CRYPTO_TYPE_AES128:
            return DESFIRE_COMMS_CIPHERTEXT_AES128;
        default:
            return DESFIRE_COMMS_PLAINTEXT;
    }
}

/* Code is adapted from @github/andrade/nfcjlib */
bool generateSessionKey(uint8_t *sessionKey, uint8_t *rndA, uint8_t *rndB, uint16_t cryptoType) {
    switch (cryptoType) {
        case CRYPTO_TYPE_DES:
            memcpy(sessionKey, rndA, 4);
            memcpy(sessionKey + 4, rndB, 4);
            break;
        case CRYPTO_TYPE_2KTDEA:
            memcpy(sessionKey, rndA, 4);
            memcpy(sessionKey + 4, rndB, 4);
            memcpy(sessionKey + 8, rndA + 4, 4);
            memcpy(sessionKey + 12, rndB + 4, 4);
            break;
        case CRYPTO_TYPE_3K3DES:
            memcpy(sessionKey, rndA, 4);
            memcpy(sessionKey + 4, rndB, 4);
            memcpy(sessionKey + 8, rndA + 6, 4);
            memcpy(sessionKey + 12, rndB + 6, 4);
            memcpy(sessionKey + 16, rndA + 12, 4);
            memcpy(sessionKey + 20, rndB + 12, 4);
            break;
        case CRYPTO_TYPE_AES128:
            memcpy(sessionKey, rndA, 4);
            memcpy(sessionKey + 4, rndB, 4);
            memcpy(sessionKey + 8, rndA + 12, 4);
            memcpy(sessionKey + 12, rndB + 12, 4);
            break;
        default:
            return false;
    }
    return true;
}

BYTE GetCryptoKeyTypeFromAuthenticateMethod(BYTE authCmdMethod) {
    switch (authCmdMethod) {
        case CMD_AUTHENTICATE_AES:
        case CMD_AUTHENTICATE_EV2_FIRST:
        case CMD_AUTHENTICATE_EV2_NONFIRST:
            return CRYPTO_TYPE_AES128;
        case CMD_AUTHENTICATE_ISO:
            return CRYPTO_TYPE_DES;
        case CMD_AUTHENTICATE:
        case CMD_ISO7816_EXTERNAL_AUTHENTICATE:
        case CMD_ISO7816_INTERNAL_AUTHENTICATE:
        default:
            return CRYPTO_TYPE_2KTDEA;
    }
}

void InitAESCryptoKeyData(void) {
    memset(&SessionKey[0], 0x00, CRYPTO_MAX_KEY_SIZE);
    memset(&SessionIV[0], 0x00, CRYPTO_MAX_BLOCK_SIZE);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
