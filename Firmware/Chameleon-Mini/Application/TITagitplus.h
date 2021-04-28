/*
 * TITagitplus.h
 *
 *  Created 20210409
 *      Author: leandre84
 *      Based on TI Tag-it Standard implementation
 */

#ifndef TITAGITPLUS_H_
#define TITAGITPLUS_H_

#include "Application.h"
#include "ISO15693-A.h"

#define TITAGIT_PLUS_UID_SIZE            ISO15693_GENERIC_UID_SIZE  //ISO15693_UID_SIZE
#define TITAGIT_PLUS_MEM_SIZE            0x110          //TAG-IT PLUS MAX MEM SIZE
#define TITAGIT_PLUS_BYTES_PER_PAGE      4
#define TITAGIT_PLUS_NUMBER_OF_SECTORS   ( TITAGIT_PLUS_MEM_SIZE / TITAGIT_PLUS_BYTES_PER_PAGE )
#define TITAGIT_PLUS_NUMBER_OF_USER_SECTORS   ( TITAGIT_PLUS_NUMBER_OF_SECTORS - 4 )
#define TITAGIT_PLUS_MEM_UID_ADDRESS     0x100        // UID byte address (two pages)
#define TITAGIT_PLUS_MEM_DSFID_ADDRESS   0x108        // DSFID byte address
#define TITAGIT_PLUS_MEM_AFI_ADDRESS     0x10C        // AFI byte address

void TITagitplusAppInit(void);
void TITagitplusAppReset(void);
void TITagitplusAppTask(void);
void TITagitplusAppTick(void);
uint16_t TITagitplusAppProcess(uint8_t *FrameBuf, uint16_t FrameBytes);
void TITagitplusGetUid(ConfigurationUidType Uid);
void TITagitplusSetUid(ConfigurationUidType Uid);
void TITagitplusFlipUid(ConfigurationUidType Uid);

#endif /* TITAGITPLUS_H_ */
