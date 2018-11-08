/*
 * MifareClassic.h
 *
 *  Created on: 01.03.2017
 *      Author: skuser
 *  modified by rickventura   
 */

#ifndef TITAGITSTANDARD_H_
#define TITAGITSTANDARD_H_

#include "Application.h"
#include "ISO15693-A.h"

#define TITAGIT_STD_UID_SIZE    ISO15693_GENERIC_UID_SIZE  //ISO15693_UID_SIZE
#define TITAGIT_STD_MEM_SIZE    44 //TAG-IT STANDARD MAX MEM SIZE

void TITagitstandardAppInit(void);
void TITagitstandardAppReset(void);
void TITagitstandardAppTask(void);
void TITagitstandardAppTick(void);
uint16_t TITagitstandardAppProcess(uint8_t* FrameBuf, uint16_t FrameBytes);
void TITagitstandardGetUid(ConfigurationUidType Uid);
void TITagitstandardSetUid(ConfigurationUidType Uid);



#endif /* VICINITY_H_ */
