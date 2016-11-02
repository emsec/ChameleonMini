/*
 * MifareDesfire.h
 * MIFARE DESFire frontend
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#ifndef MIFAREDESFIRE_H_
#define MIFAREDESFIRE_H_

#include "Application.h"

void MifareDesfireEV0AppInit(void);
void MifareDesfire2kEV1AppInit(void);
void MifareDesfire4kEV1AppInit(void);
void MifareDesfire8kEV1AppInit(void);
void MifareDesfireAppReset(void);
void MifareDesfireAppTask(void);

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount);

void MifareDesfireGetUid(ConfigurationUidType Uid);
void MifareDesfireSetUid(ConfigurationUidType Uid);

#endif /* MIFAREDESFIRE_H_ */
