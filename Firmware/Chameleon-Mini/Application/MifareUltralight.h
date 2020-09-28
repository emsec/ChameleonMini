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

#define MIFARE_ULTRALIGHTC_UID_SIZE    ISO14443A_UID_SIZE_DOUBLE
#define MIFARE_ULTRALIGHTC_PAGE_SIZE   4
#define MIFARE_ULTRALIGHTC_PAGES      48
#define MIFARE_ULTRALIGHTC_MEM_SIZE			(MIFARE_ULTRALIGHTC_PAGES * MIFARE_ULTRALIGHTC_PAGE_SIZE)

#define MIFARE_ULTRALIGHT_UID_SIZE    ISO14443A_UID_SIZE_DOUBLE
#define MIFARE_ULTRALIGHT_PAGE_SIZE   4
#define MIFARE_ULTRALIGHT_PAGES       16
#define MIFARE_ULTRALIGHT_EV11_PAGES  20
#define MIFARE_ULTRALIGHT_EV12_PAGES  41
#define MIFARE_ULTRALIGHT_NTAG_215_PAGES       135 //135 pages total, from 0 to 134
#define MIFARE_ULTRALIGHT_MEM_SIZE          (MIFARE_ULTRALIGHT_PAGES * MIFARE_ULTRALIGHT_PAGE_SIZE)
#define MIFARE_ULTRALIGHT_EV11_MEM_SIZE     (MIFARE_ULTRALIGHT_EV11_PAGES * MIFARE_ULTRALIGHT_PAGE_SIZE)
#define MIFARE_ULTRALIGHT_EV12_MEM_SIZE     (MIFARE_ULTRALIGHT_EV12_PAGES * MIFARE_ULTRALIGHT_PAGE_SIZE)
#define MIFARE_ULTRALIGHT_NTAG_215_MEM_SIZE    ( MIFARE_ULTRALIGHT_NTAG_215_PAGES * MIFARE_ULTRALIGHTC_PAGE_SIZE )

void MifareUltralightAppInit(void);
void MifareUltralightEV11AppInit(void);
void MifareUltralightEV12AppInit(void);
void MifareUltralightNTAG215AppInit(void);
void MifareUltralightAppReset(void);
void MifareUltralightAppTask(void);

void MifareUltralightCAppInit(void);
void MifareUltralightCAppReset(void);

uint16_t MifareUltralightAppProcess(uint8_t *Buffer, uint16_t BitCount);

void MifareUltralightGetUid(ConfigurationUidType Uid);
void MifareUltralightSetUid(ConfigurationUidType Uid);



#endif /* MIFAREULTRALIGHT_H_ */
