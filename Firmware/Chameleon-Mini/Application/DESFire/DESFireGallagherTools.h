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

#ifndef CHAMELEON_MINI_DESFIREGALLAGHERTOOLS_H
#define CHAMELEON_MINI_DESFIREGALLAGHERTOOLS_H

#include "DESFireCrypto.h"
#include "DESFireApplicationDirectory.h"
#include "Configuration.h"
#include "DESFirePICCControl.h"
#include "DESFireStatusCodes.h"

//bool MifareKdfAn10922(uint8_t *key, uint8_t CryptoType, const uint8_t *data, size_t len, uint8_t *diversified_key);
//uint8_t mfdes_kdf_input_gallagher(uint8_t *uid, uint8_t uidLen, uint8_t keyNo, uint32_t aid, uint8_t *kdfInputOut, uint8_t *kdfInputLen);
bool hfgal_diversify_key(uint8_t *site_key, uint8_t *uid, uint8_t uid_len, uint8_t key_num, DESFireAidType aid, uint8_t *key_output);

//void scramble(uint8_t *arr, uint8_t len);
void gallagher_encode_creds(uint8_t *eight_bytes, uint8_t rc, uint16_t fc, uint32_t cn, uint8_t il);

#endif //CHAMELEON_MINI_DESFIREGALLAGHERTOOLS_H
