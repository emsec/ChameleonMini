//
// Created by Tom on 13.11.2022.
//

#ifndef CHAMELEON_MINI_DESFIREGALLAGHERTOOLS_H
#define CHAMELEON_MINI_DESFIREGALLAGHERTOOLS_H

#include "DESFireCrypto.h"
#include "DESFireApplicationDirectory.h"
#include "Configuration.h"
#include "DESFirePICCControl.h"
#include "DESFireStatusCodes.h"

bool MifareKdfAn10922(uint8_t *key, uint8_t CryptoType, const uint8_t *data, size_t len, uint8_t *diversified_key);
int mfdes_kdf_input_gallagher(uint8_t *uid, uint8_t uidLen, uint8_t keyNo, uint32_t aid, uint8_t *kdfInputOut, uint8_t *kdfInputLen);
bool hfgal_diversify_key(uint8_t *site_key, uint8_t *uid, uint8_t uid_len, uint8_t key_num, uint32_t aid, uint8_t *key_output);

void scramble(uint8_t *arr, uint8_t len);
void gallagher_encode_creds(uint8_t *eight_bytes, uint8_t rc, uint16_t fc, uint32_t cn, uint8_t il);

#endif //CHAMELEON_MINI_DESFIREGALLAGHERTOOLS_H
