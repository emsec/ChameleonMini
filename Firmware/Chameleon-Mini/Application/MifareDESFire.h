/*
The DESFire stack portion of this firmware source
is free software written by Maxie Dion Schmidt (@maxieds):
You can redistribute it and/or modify
it under the terms of this license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

The complete source distribution of
this firmware is available at the following link:
https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.

Based in part on the original DESFire code created by
@dev-zzo (GitHub handle) [Dmitry Janushkevich] available at
https://github.com/dev-zzo/ChameleonMini/tree/desfire.

This notice must be retained at the top of all source files where indicated.
*/

/*
 * MifareDesfire.h
 * MIFARE DESFire frontend
 *
 * Created on: 14.10.2016
 * Author: dev_zzo
 */

#ifndef MIFAREDESFIRE_H_
#define MIFAREDESFIRE_H_

#include "Application.h"
#include "DESFire/DESFireFirmwareSettings.h"
#include "DESFire/DESFirePICCHeaderLayout.h"
#include "DESFire/DESFireISO14443Support.h"
#include "DESFire/DESFireISO7816Support.h"
#include "DESFire/DESFireInstructions.h"

#define DesfireCLA(cmdCode) \
    ((cmdCode == DESFIRE_NATIVE_CLA) || Iso7816CLA(cmdCode))

void MifareDesfireEV0AppInit(void);
void MifareDesfireEV0AppInitRunOnce(void);
void MifareDesfire2kEV1AppInit(void);
void MifareDesfire2kEV1AppInitRunOnce(void);
void MifareDesfire4kEV1AppInit(void);
void MifareDesfire4kEV1AppInitRunOnce(void);
void MifareDesfire4kEV2AppInit(void);
void MifareDesfire4kEV2AppInitRunOnce(void);

void MifareDesfireAppReset(void);
void MifareDesfireAppTick(void);
void MifareDesfireAppTask(void);

uint16_t MifareDesfireProcessCommand(uint8_t *Buffer, uint16_t ByteCount);
uint16_t MifareDesfireProcess(uint8_t *Buffer, uint16_t ByteCount);
uint16_t MifareDesfireAppProcess(uint8_t *Buffer, uint16_t BitCount);

void MifareDesfireReset(void);
void ResetLocalStructureData(void);
void ResetISOState(void);

void MifareDesfireGetUid(ConfigurationUidType Uid);
void MifareDesfireSetUid(ConfigurationUidType Uid);

typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
    DESFIRE_HALT = ISO14443_3A_STATE_HALT + 1,
    DESFIRE_IDLE,
    DESFIRE_GET_VERSION2,
    DESFIRE_GET_VERSION3,
    DESFIRE_GET_APPLICATION_IDS2,
    DESFIRE_LEGACY_AUTHENTICATE,
    DESFIRE_LEGACY_AUTHENTICATE2,
    DESFIRE_ISO_AUTHENTICATE,
    DESFIRE_ISO_AUTHENTICATE2,
    DESFIRE_AES_AUTHENTICATE,
    DESFIRE_AES_AUTHENTICATE2,
    DESFIRE_ISO7816_EXT_AUTH,
    DESFIRE_ISO7816_INT_AUTH,
    DESFIRE_ISO7816_GET_CHALLENGE,
    DESFIRE_READ_DATA_FILE,
    DESFIRE_WRITE_DATA_FILE,
} DesfireStateType;

#define DesfireStateExpectingAdditionalFrame(dfState)  \
	((dfState == DESFIRE_GET_VERSION2)          || \
	 (dfState == DESFIRE_GET_VERSION3)          || \
	 (dfState == DESFIRE_GET_APPLICATION_IDS2)  || \
	 (dfState == DESFIRE_LEGACY_AUTHENTICATE2)  || \
	 (dfState == DESFIRE_ISO_AUTHENTICATE2)     || \
	 (dfState == DESFIRE_AES_AUTHENTICATE2)     || \
	 (dfState == DESFIRE_READ_DATA_FILE)        || \
	 (dfState == DESFIRE_WRITE_DATA_FILE))

#define MutualAuthenticateCmd(cmdCode)                     \
	((cmdCode == CMD_ISO7816_EXTERNAL_AUTHENTICATE) || \
	 (cmdCode == CMD_ISO7816_INTERNAL_AUTHENTICATE) || \
	 (cmdCode == CMD_ISO7816_GET_CHALLENGE))

extern DesfireStateType DesfireState;
extern DesfireStateType DesfirePreviousState;

extern Iso7816WrappedCommandType_t Iso7816CmdType;

extern bool DesfireFromHalt;
extern BYTE DesfireCmdCLA;

#endif /* MIFAREDESFIRE_H_ */
