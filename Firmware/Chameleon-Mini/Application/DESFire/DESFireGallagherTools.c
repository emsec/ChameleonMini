// This file contains adapted Gallagher functions from https://github.com/RfidResearchGroup/proxmark3
// All credit goes to this repo

//-----------------------------------------------------------------------------
// Copyright (C) Proxmark3 contributors. See AUTHORS.md @ https://github.com/RfidResearchGroup/proxmark3 for details.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// See LICENSE.txt @ https://github.com/RfidResearchGroup/proxmark3 for the text of the license.

#include <string.h>
#include "DESFireGallagherTools.h"
#include "DESFireLogging.h"
#include "DESFireUtils.h"

// This function is almot like cmac(...). but with some key differences.
bool MifareKdfAn10922(uint8_t *key, uint8_t CryptoType,
                      const uint8_t *data, size_t len, uint8_t *diversified_key) {
    if (data == NULL || len < 1 || len > 31)
        return false;

    uint8_t kbs;
    switch (CryptoType) {
        case CRYPTO_TYPE_AES128:
            kbs = CRYPTO_AES_BLOCK_SIZE;
            break;
        case CRYPTO_TYPE_2KTDEA:
            kbs = CRYPTO_2KTDEA_BLOCK_SIZE;
            break;
        case CRYPTO_TYPE_3K3DES:
            kbs = CRYPTO_3KTDEA_BLOCK_SIZE;
            break;
        default:
            return false;
    }

    uint8_t buffer[96] = {0};

    if (CryptoType == CRYPTO_TYPE_AES128) {
        buffer[0] = 0x01;
        memcpy(&buffer[1], data, len);

        uint8_t IV[CRYPTO_AES_BLOCK_SIZE];
        memset(IV, 0, CRYPTO_AES_BLOCK_SIZE);
        DesfireCryptoCMACEx(CryptoType, key, buffer, len + 1, IV, diversified_key, 2*kbs);

        return true;
    } else if (CryptoType == CRYPTO_TYPE_2KTDEA) {
        /* TODO: Adapt this code
        buffer[0] = 0x21;
        memcpy(&buffer[1], data, len);

        DesfireClearIV(ctx);
        DesfireCryptoCMACEx(ctx, key_type, buffer, len + 1, kbs * 2, cmac);

        buffer[0] = 0x22;
        memcpy(&buffer[1], data, len);

        DesfireClearIV(ctx);
        DesfireCryptoCMACEx(ctx, key_type, buffer, len + 1, kbs * 2, &cmac[kbs]);

        memcpy(ctx->key, cmac, kbs * 2);*/
    } else if (CryptoType == CRYPTO_TYPE_3K3DES) {
        /* TODO: Adapt this code
        buffer[0] = 0x31;
        memcpy(&buffer[1], data, len);

        DesfireClearIV(ctx);
        DesfireCryptoCMACEx(ctx, key_type, buffer, len + 1, kbs * 2, cmac);

        buffer[0] = 0x32;
        memcpy(&buffer[1], data, len);

        DesfireClearIV(ctx);
        DesfireCryptoCMACEx(ctx, key_type, buffer, len + 1, kbs * 2, &cmac[kbs]);

        buffer[0] = 0x33;
        memcpy(&buffer[1], data, len);

        DesfireClearIV(ctx);
        DesfireCryptoCMACEx(ctx, key_type, buffer, len + 1, kbs * 2, &cmac[kbs * 2]);

        memcpy(ctx->key, cmac, kbs * 3);*/
    }
    return false;
}

uint8_t mfdes_kdf_input_gallagher(uint8_t *uid, uint8_t uidLen, uint8_t keyNo,
                                  DESFireAidType aid, uint8_t *kdfInputOut, uint8_t *kdfInputLen) {
    if (uid == NULL || (uidLen != 4 && uidLen != 7) || keyNo > 2 || kdfInputOut == NULL || kdfInputLen == NULL) {
        return 1;
    }

    int len = 0;
    // If the keyNo == 1 or the aid is 000000, then omit the UID.
    // On the other hand, if the aid is 1f81f4 (config card) always include the UID.
    if ((keyNo != 1 && aid != 0x000000) || (aid[0] == 0x1f && aid[1] == 0x81 && aid[2] == 0xf4)) {
        if (*kdfInputLen < (4 + uidLen)) {
            return 2;
        }

        memcpy(kdfInputOut, uid, uidLen);
        len += uidLen;
    } else if (*kdfInputLen < 4) {
        return 4;
    }

    kdfInputOut[len++] = keyNo;

    kdfInputOut[len++] = aid[0];
    kdfInputOut[len++] = aid[1];
    kdfInputOut[len++] = aid[2];

    *kdfInputLen = len;

    return 0;
}

bool hfgal_diversify_key(uint8_t *site_key, uint8_t *uid, uint8_t uid_len,
                        uint8_t key_num, DESFireAidType aid, uint8_t *key_output) {
    // Generate diversification input
    uint8_t kdf_input_len = 11;
    uint32_t res = mfdes_kdf_input_gallagher(uid, uid_len, key_num, aid, key_output, &kdf_input_len);
    if (res) {
        return false;
    }

    uint8_t key[CRYPTO_AES_KEY_SIZE];
    if (site_key == NULL) {
        return false;
    }
    memcpy(key, site_key, sizeof(key));

    // Diversify input & copy to output buffer
    uint8_t diversified_key[CRYPTO_AES_KEY_SIZE];
    bool ret = MifareKdfAn10922(key, CRYPTO_TYPE_AES128, key_output, kdf_input_len, diversified_key);
    memcpy(key_output, diversified_key, CRYPTO_AES_KEY_SIZE);

    return ret;
}

void scramble(uint8_t *arr, uint8_t len) {
    const uint8_t lut[] = {
            0xa3, 0xb0, 0x80, 0xc6, 0xb2, 0xf4, 0x5c, 0x6c, 0x81, 0xf1, 0xbb, 0xeb, 0x55, 0x67, 0x3c, 0x05,
            0x1a, 0x0e, 0x61, 0xf6, 0x22, 0xce, 0xaa, 0x8f, 0xbd, 0x3b, 0x1f, 0x5e, 0x44, 0x04, 0x51, 0x2e,
            0x4d, 0x9a, 0x84, 0xea, 0xf8, 0x66, 0x74, 0x29, 0x7f, 0x70, 0xd8, 0x31, 0x7a, 0x6d, 0xa4, 0x00,
            0x82, 0xb9, 0x5f, 0xb4, 0x16, 0xab, 0xff, 0xc2, 0x39, 0xdc, 0x19, 0x65, 0x57, 0x7c, 0x20, 0xfa,
            0x5a, 0x49, 0x13, 0xd0, 0xfb, 0xa8, 0x91, 0x73, 0xb1, 0x33, 0x18, 0xbe, 0x21, 0x72, 0x48, 0xb6,
            0xdb, 0xa0, 0x5d, 0xcc, 0xe6, 0x17, 0x27, 0xe5, 0xd4, 0x53, 0x42, 0xf3, 0xdd, 0x7b, 0x24, 0xac,
            0x2b, 0x58, 0x1e, 0xa7, 0xe7, 0x86, 0x40, 0xd3, 0x98, 0x97, 0x71, 0xcb, 0x3a, 0x0f, 0x01, 0x9b,
            0x6e, 0x1b, 0xfc, 0x34, 0xa6, 0xda, 0x07, 0x0c, 0xae, 0x37, 0xca, 0x54, 0xfd, 0x26, 0xfe, 0x0a,
            0x45, 0xa2, 0x2a, 0xc4, 0x12, 0x0d, 0xf5, 0x4f, 0x69, 0xe0, 0x8a, 0x77, 0x60, 0x3f, 0x99, 0x95,
            0xd2, 0x38, 0x36, 0x62, 0xb7, 0x32, 0x7e, 0x79, 0xc0, 0x46, 0x93, 0x2f, 0xa5, 0xba, 0x5b, 0xaf,
            0x52, 0x1d, 0xc3, 0x75, 0xcf, 0xd6, 0x4c, 0x83, 0xe8, 0x3d, 0x30, 0x4e, 0xbc, 0x08, 0x2d, 0x09,
            0x06, 0xd9, 0x25, 0x9e, 0x89, 0xf2, 0x96, 0x88, 0xc1, 0x8c, 0x94, 0x0b, 0x28, 0xf0, 0x47, 0x63,
            0xd5, 0xb3, 0x68, 0x56, 0x9c, 0xf9, 0x6f, 0x41, 0x50, 0x85, 0x8b, 0x9d, 0x59, 0xbf, 0x9f, 0xe2,
            0x8e, 0x6a, 0x11, 0x23, 0xa1, 0xcd, 0xb5, 0x7d, 0xc7, 0xa9, 0xc8, 0xef, 0xdf, 0x02, 0xb8, 0x03,
            0x6b, 0x35, 0x3e, 0x2c, 0x76, 0xc9, 0xde, 0x1c, 0x4b, 0xd1, 0xed, 0x14, 0xc5, 0xad, 0xe9, 0x64,
            0x4a, 0xec, 0x8d, 0xf7, 0x10, 0x43, 0x78, 0x15, 0x87, 0xe4, 0xd7, 0x92, 0xe1, 0xee, 0xe3, 0x90
    };

    for (int i = 0; i < len;  i++) {
        arr[i] = lut[arr[i]];
    }
}

void gallagher_encode_creds(uint8_t *eight_bytes, uint8_t rc, uint16_t fc, uint32_t cn, uint8_t il) {
    // put data into the correct places (Gallagher obfuscation)
    eight_bytes[0] = (cn & 0xffffff) >> 16;
    eight_bytes[1] = (fc & 0xfff) >> 4;
    eight_bytes[2] = (cn & 0x7ff) >> 3;
    eight_bytes[3] = (cn & 0x7) << 5 | (rc & 0xf) << 1;
    eight_bytes[4] = (cn & 0xffff) >> 11;
    eight_bytes[5] = (fc & 0xffff) >> 12;
    eight_bytes[6] = 0;
    eight_bytes[7] = (fc & 0xf) << 4 | (il & 0xf);

    // more obfuscation
    scramble(eight_bytes, 8);
}
