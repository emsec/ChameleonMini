/*
 * NTAG21x.h
 *
 *  Created on: 20.02.2019
 *      Author: gcammisa
 */

#ifndef NTAG21x_H_
#define NTAG21x_H_

#include "Application.h"
#include "ISO14443-3A.h"

#define NTAG21x_UID_SIZE ISO14443A_UID_SIZE_DOUBLE //7 bytes UID
#define NTAG21x_PAGE_SIZE 4 //bytes per page

#define NTAG213_PAGES 45 //45 pages total, from 0 to 44
#define NTAG213_MEM_SIZE ( NTAG21x_PAGE_SIZE * NTAG213_PAGES )

#define NTAG215_PAGES 135 //135 pages total, from 0 to 134
#define NTAG215_MEM_SIZE ( NTAG21x_PAGE_SIZE * NTAG215_PAGES )

#define NTAG216_PAGES 231 //231 pages total, from 0 to 230
#define NTAG216_MEM_SIZE ( NTAG21x_PAGE_SIZE * NTAG216_PAGES )

void NTAG213AppInit(void);

void NTAG215AppInit(void);

void NTAG216AppInit(void);

void NTAG21xAppReset(void);
void NTAG21xAppTask(void);

uint16_t NTAG21xAppProcess(uint8_t *Buffer, uint16_t BitCount);

void NTAG21xGetUid(ConfigurationUidType Uid);
void NTAG21xSetUid(ConfigurationUidType Uid);
#endif