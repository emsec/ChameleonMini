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

#define TITAGIT_STD_UID_SIZE        ISO15693_GENERIC_UID_SIZE  //ISO15693_UID_SIZE
#define TITAGIT_STD_MEM_SIZE        44          //TAG-IT STANDARD MAX MEM SIZE
#define TITAGIT_BYTES_PER_PAGE      4
#define TITAGIT_NUMBER_OF_SECTORS   ( TITAGIT_STD_MEM_SIZE / TITAGIT_BYTES_PER_PAGE )
#define TITAGIT_MEM_UID_ADDRESS     0x20
#define TITAGIT_MEM_AFI_ADDRESS     0x28        // AFI byte address

void TITagitstandardAppInit(void);
void TITagitstandardAppReset(void);
void TITagitstandardAppTask(void);
void TITagitstandardAppTick(void);
uint16_t TITagitstandardAppProcess(uint8_t *FrameBuf, uint16_t FrameBytes);
void TITagitstandardGetUid(ConfigurationUidType Uid);
void TITagitstandardSetUid(ConfigurationUidType Uid);
void TITagitstandardFlipUid(ConfigurationUidType Uid);

#endif /* TITAGITSTANDARD_H_ */
