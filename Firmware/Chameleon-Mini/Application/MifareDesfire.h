/*
 * MifareDesfire.h
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#ifndef MIFAREDESFIRE_H_
#define MIFAREDESFIRE_H_

#include "Application.h"
#include "ISO14443-3A.h"

#define MIFARE_DESFIRE_UID_SIZE     ISO14443A_UID_SIZE_DOUBLE

void MifareClassicAppInit1K(void);
void MifareClassicAppInit4K(void);
void MifareClassicAppReset(void);
void MifareClassicAppTask(void);

uint16_t MifareClassicAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareClassicGetUid(ConfigurationUidType Uid);
void MifareClassicSetUid(ConfigurationUidType Uid);


#endif /* MIFAREDESFIRE_H_ */
