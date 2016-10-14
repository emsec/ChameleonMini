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

/* Anticollision parameters */
#define ATQA_VALUE              0x0344
#define SAK_CL1_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT | ISO14443A_SAK_INCOMPLETE)
#define SAK_CL2_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT)

/* UID location in the card memory; sizes are in bytes */
#define UID_CL1_ADDRESS         0x00
#define UID_CL1_SIZE            3
#define UID_BCC1_ADDRESS        0x03
#define UID_CL2_ADDRESS         0x04
#define UID_CL2_SIZE            4
#define UID_BCC2_ADDRESS        0x08


typedef enum {
    /* Something went wrong or we've received a halt command */
    STATE_HALT,
    /* The card is powered up but not selected */
    STATE_IDLE,
    /* Entered on REQA or WUPA; anticollision is being performed */
    STATE_READY1,
    STATE_READY2,
    /* Entered when the card is selected */
    STATE_ACTIVE,
} StateType;

static StateType State;

/*
 * Application code
 */

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
    switch(State) {
    case STATE_IDLE:
    case STATE_HALT:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE)) {
            /* We received a REQA or WUPA command, so wake up. */
            State = STATE_READY1;
            return BitCount;
        }
        break;

    case STATE_READY1:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE)) {
            State = STATE_READY1;
            return BitCount;
        }
        if (Buffer[0] == ISO14443A_CMD_SELECT_CL1) {
            /* Load UID CL1 and perform anticollision. */
            uint8_t UidCL1[ISO14443A_CL_UID_SIZE];

            UidCL1[0] = ISO14443A_UID0_CT;
            MemoryReadBlock(&UidCL1[1], UID_CL1_ADDRESS, UID_CL1_SIZE);
            if (ISO14443ASelect(Buffer, &BitCount, UidCL1, SAK_CL1_VALUE)) {
                /* CL1 stage has ended successfully */
                State = STATE_READY2;
            }

            return BitCount;
        }
        break;

    case STATE_READY2:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE)) {
            State = STATE_READY1;
            return BitCount;
        }
        if (Cmd == ISO14443A_CMD_SELECT_CL2) {
            /* Load UID CL2 and perform anticollision */
            uint8_t UidCL2[ISO14443A_CL_UID_SIZE];

            MemoryReadBlock(UidCL2, UID_CL2_ADDRESS, UID_CL2_SIZE);
            if (ISO14443ASelect(Buffer, &BitCount, UidCL2, SAK_CL2_VALUE)) {
                /* CL2 stage has ended successfully. This means
                 * our complete UID has been sent to the reader. */
                State = STATE_ACTIVE;
            }

            return BitCount;
        }
        break;

    case STATE_ACTIVE:
        break;
    }

    /* Unknown command. Enter halt state */
    State = STATE_HALT;
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

