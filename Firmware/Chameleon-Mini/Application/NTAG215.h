/*
 * NTAG215.h
 *
 *  Created on: 20.02.2019
 *      Author: gcammisa
 */

#ifndef NTAG215_H_
#define NTAG215_H_

#include "Application.h"
#include "ISO14443-3A.h"

#define NTAG215_UID_SIZE ISO14443A_UID_SIZE_DOUBLE //7 bytes UID
#define NTAG215_PAGE_SIZE 4 //bytes per page
#define NTAG215_PAGES 135 //135 pages total, from 0 to 134
#define NTAG215_MEM_SIZE ( NTAG215_PAGE_SIZE * NTAG215_PAGES )

void NTAG215AppInit(void);
void NTAG215AppReset(void);
void NTAG215AppTask(void);

uint16_t NTAG215AppProcess(uint8_t *Buffer, uint16_t BitCount);

void NTAG215GetUid(ConfigurationUidType Uid);
void NTAG215SetUid(ConfigurationUidType Uid);
#endif
