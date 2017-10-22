/*
 * MifareUltralight.h
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#ifndef MIFAREULTRALIGHT_H_
#define MIFAREULTRALIGHT_H_

#include "Application.h"
#include "ISO14443-3A.h"

#define MIFARE_ULTRALIGHT_UID_SIZE    ISO14443A_UID_SIZE_DOUBLE
#define MIFARE_ULTRALIGHT_MEM_SIZE    64

void MifareUltralightAppInit(void);
void MifareUltralightAppReset(void);
void MifareUltralightAppTask(void);

uint16_t MifareUltralightAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareUltralightGetUid(ConfigurationUidType Uid);
void MifareUltralightSetUid(ConfigurationUidType Uid);



#endif /* MIFAREULTRALIGHT_H_ */
