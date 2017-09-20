/*
 * MifareClassic.h
 *
 *  Created on: 01.03.2017
 *      Author: skuser
 */

#ifndef SL2S2002_H_
#define SL2S2002_H_

#include "Application.h"
#include "ISO15693-A.h"

#define ISO15693_GENERIC_UID_SIZE    8 //ISO15693_UID_SIZE
#define ISO15693_GENERIC_MEM_SIZE    8192 //ISO15693_MAX_MEM_SIZE

void Sl2s2002AppInit(void);
void Sl2s2002AppReset(void);
void Sl2s2002AppTask(void);
void Sl2s2002AppTick(void);
uint16_t Sl2s2002AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes);
void Sl2s2002GetUid(ConfigurationUidType Uid);
void Sl2s2002SetUid(ConfigurationUidType Uid);



#endif /* VICINITY_H_ */
