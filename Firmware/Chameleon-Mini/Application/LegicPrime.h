/*
 * LegicPrime.h
 *
 *  Created on: 5.7.2024
 *      Author: Ladislav Marko
 *  Inspired by MifareClassic.h
 */

#ifndef LEGIC_PRIME_H
#define LEGIC_PRIME_H
#include "Application.h"

#define LEGIC_PRIME_UID_SIZE       4
#define LEGIC_PRIME_MEM_SIZE       256 // There are two main LEGIC prime variants -- 256 and 1024 bytes, so we use the small one now

void LegicPrimeAppInit(void);
void LegicPrimeAppReset(void);

uint16_t LegicPrimeAppProcess(uint8_t *Buffer, uint16_t BitCount);

void LegicPrimeGetUid(ConfigurationUidType Uid);
void LegicPrimeSetUid(ConfigurationUidType Uid);
#endif //LEGIC_PRIME_H
