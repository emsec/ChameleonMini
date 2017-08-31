/*
 * MifareClassic.h
 *
 *  Created on: 01.03.2017
 *      Author: skuser
 */

#ifndef VICINITY_H_
#define VICINITY_H_

#include "Application.h"
#include "ISO15693-A.h"

#define ISO15693_GENERIC_UID_SIZE    8 //ISO15693_UID_SIZE
#define ISO15693_GENERIC_MEM_SIZE    8192 //ISO15693_MAX_MEM_SIZE

void VicinityAppInit(void);
void VicinityAppReset(void);
void VicinityAppTask(void);
void VicinityAppTick(void);
uint16_t VicinityAppProcess(uint8_t* FrameBuf, uint16_t FrameBytes);
void VicinityGetUid(ConfigurationUidType Uid);
void VicinitySetUid(ConfigurationUidType Uid);



#endif /* VICINITY_H_ */
