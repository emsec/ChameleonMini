/*
 * MifareDesfire.c
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#include "MifareDesfire.h"

#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"
#include "../Random.h"

typedef enum {
    STATE_HALT,
    STATE_IDLE,
    STATE_READY1,
    STATE_READY2,
    STATE_ACTIVE,
} StateType;

static StateType State;

void MifareDesfireAppInit(void)
{
    State = STATE_IDLE;
}

void MifareDesfireAppReset(void)
{
    State = STATE_IDLE;
}

void MifareDesfireAppTask(void)
{
    /* Empty */
}

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
    return ISO14443A_APP_NO_RESPONSE;
}

void MifareDesfireGetUid(ConfigurationUidType Uid)
{
    MemoryReadBlock(&Uid[0], MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE-1);
    MemoryReadBlock(&Uid[3], MEM_UID_CL2_ADDRESS, MEM_UID_CL2_SIZE);
}

void MifareDesfireSetUid(ConfigurationUidType Uid)
{
    MemoryWriteBlock(Uid, MEM_UID_CL1_ADDRESS, ActiveConfiguration.UidSize);
}

