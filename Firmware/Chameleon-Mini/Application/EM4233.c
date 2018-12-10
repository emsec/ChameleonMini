/*
 * EM4233.c
 *
 *  Created on: 12-05-2018
 *      Author: ceres-c & MrMoDDoM
 */

#include "ISO15693-A.h"
#include "EM4233.h"

static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

CurrentFrame FrameInfo;

uint64_t EM4233_FactoryLockBits_Mask = 0;  /* Holds lock state of blocks */
uint64_t EM4233_UserLockBits_Mask = 0;     /* Holds lock state of blocks */

//uint8_t EM4233_Lock_Status[64] = { 1 };


void EM4233AppInit(void)
{
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;

    //MemoryReadBlock(EM4233_Lock_Status, EM4233_MEM_LSM_ADDRESS, 64);
    //MemoryReadBlock(&EM4233_FactoryLockBits_Mask, EM4233_MEM_FLM_ADDRESS, 8);
}

void EM4233AppReset(void)
{
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;

    //MemoryReadBlock(&EM4233_UserLockBits_Mask, EM4233_MEM_ULM_ADDRESS, 8);
    //MemoryReadBlock(&EM4233_FactoryLockBits_Mask, EM4233_MEM_FLM_ADDRESS, 8);
}


void EM4233AppTask(void)
{

}

void EM4233AppTick(void)
{

}

uint16_t EM4233_Lock_Block(uint8_t* FrameBuf, uint16_t FrameBytes) 
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t PageAddress = *FrameInfo.Parameters;

    if (FrameInfo.ParamLen != 1)
         return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (PageAddress > EM4233_NUMBER_OF_BLCKS) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = 2;
        return ResponseByteCount; /* malformed: trying to lock a non-existing block */
    }

    if ((EM4233_FactoryLockBits_Mask & (uint64_t)(1 << PageAddress)) || (EM4233_UserLockBits_Mask & (uint64_t)(1 << PageAddress))) { /* if already locked */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_ALRD_LKD;
        ResponseByteCount = 2;
    } else {
        EM4233_UserLockBits_Mask |= (uint64_t)(1 << PageAddress); /* write the lock status in mask */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount = 1;
    }

    return ResponseByteCount;
}

uint16_t EM4233_Write_Single(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t* Dataptr;
    uint8_t PageAddress = *FrameInfo.Parameters;

    if (FrameInfo.ParamLen != 5)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (PageAddress > EM4233_NUMBER_OF_BLCKS) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = 2; /* malformed: trying to write in a non-existing block */
        return ResponseByteCount;
    }

    Dataptr = PageAddress + 0x01;

    if (EM4233_FactoryLockBits_Mask & (uint64_t)(1 << PageAddress)) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = 2;
    } else if (EM4233_UserLockBits_Mask & (uint64_t)(1 << PageAddress)) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_CHG_LKD;
        ResponseByteCount = 2;
    } else {
        MemoryWriteBlock(Dataptr, PageAddress * EM4233_BYTES_PER_BLCK, EM4233_BYTES_PER_BLCK);
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount = 1;
    }

    return ResponseByteCount;
}

uint16_t EM4233_Read_Single(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t *FramePtr; /* holds the address where block's data will be put */
    uint8_t PageAddress = *FrameInfo.Parameters;
    uint8_t LockStatus = 0; 

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (PageAddress >= EM4233_NUMBER_OF_BLCKS) { /* the reader is requesting a sector out of bound */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_NOT_AVL; /* real TiTag standard reply with this error */
        ResponseByteCount = 2;
        return ResponseByteCount;
    }

    if (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) { /* request with option flag set */  
        MemoryReadBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + PageAddress), 1);
        if (LockStatus & ISO15693_MASK_FACTORY_LOCK)  { /* tests if the n-th bit of the factory bitmask if set to 1 */
            FrameBuf[1] = 0x02; /* return bit 1 set as 1 (factory locked) */
        } else if (LockStatus & ISO15693_MASK_USER_LOCK) { /* tests if the n-th bit of the user bitmask if set to 1 */
            FrameBuf[1] = 0x01; /* return bit 0 set as 1 (user locked) */
        } else
            FrameBuf[1] = 0x00; /* return lock status 00 (unlocked) */
        FramePtr = FrameBuf + 2; /* block's data from byte 2 */
        ResponseByteCount = 6;
    } else { /* request with option flag not set */
        FramePtr = FrameBuf + 1; /* block's data from byte 1 */
        ResponseByteCount = 5;
    }

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    MemoryReadBlock(FramePtr, PageAddress * EM4233_BYTES_PER_BLCK, EM4233_BYTES_PER_BLCK);

    return ResponseByteCount;
}

uint16_t EM4233AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t Uid[ActiveConfiguration.UidSize];
    EM4233GetUid(Uid);

    if (!ISO15693PrepareFrame(FrameBuf, FrameBytes, &FrameInfo, Uid))
        return ISO15693_APP_NO_RESPONSE;

    switch(State) {
    case STATE_READY:
        if (*FrameInfo.Command == ISO15693_CMD_INVENTORY) {
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
            FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_INVENTORY_DSFID;
            ISO15693CopyUid(&FrameBuf[ISO15693_RES_ADDR_PARAM + 0x01], Uid);
            ResponseByteCount = 10;

        } else if (*FrameInfo.Command == ISO15693_CMD_STAY_QUIET && FrameInfo.Addressed) {
            State = STATE_QUIET;

        } else if (*FrameInfo.Command == ISO15693_CMD_READ_SINGLE) {
            ResponseByteCount = EM4233_Read_Single(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_WRITE_SINGLE) {
            ResponseByteCount = EM4233_Write_Single(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_LOCK_BLOCK) {
            ResponseByteCount = EM4233_Lock_Block(FrameBuf, FrameBytes);

        } else {
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
            FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_NOT_SUPP;
            ResponseByteCount = 2;
        }
        break;

    case STATE_SELECTED:
        /* TO-DO */
        break;

    case STATE_QUIET:
        if (*FrameInfo.Command == ISO15693_CMD_RESET_TO_READY) {
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
            ResponseByteCount = 1;
            State = STATE_READY;

            FrameInfo.Flags         = NULL;
            FrameInfo.Command       = NULL;
            FrameInfo.Parameters    = NULL;
            FrameInfo.ParamLen      = 0;
            FrameInfo.Addressed     = false;
        }

        break;

    default:
        break;
    }

    if (ResponseByteCount > 0) {
        /* There is data to be sent. Append CRC */
        ISO15693AppendCRC(FrameBuf, ResponseByteCount);
        ResponseByteCount += ISO15693_CRC16_SIZE;
    }

    return ResponseByteCount;
}

void EM4233GetUid(ConfigurationUidType Uid)
{
    MemoryReadBlock(&Uid[0], EM4233_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void EM4233SetUid(ConfigurationUidType Uid)
{
    MemoryWriteBlock(Uid, EM4233_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}