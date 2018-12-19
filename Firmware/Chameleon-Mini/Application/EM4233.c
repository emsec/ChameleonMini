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

bool loggedIn;

CurrentFrame FrameInfo;

uint64_t EM4233_FactoryLockBits_Mask = 0;  /* Holds lock state of blocks */
uint64_t EM4233_UserLockBits_Mask = 0;     /* Holds lock state of blocks */

void EM4233AppInit(void)
{
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;
    loggedIn = false;

}

void EM4233AppReset(void)
{
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;
    loggedIn = false;
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
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount; /* malformed: trying to lock a non-existing block */
    }

    if ((EM4233_FactoryLockBits_Mask & (uint64_t)(1 << PageAddress)) || (EM4233_UserLockBits_Mask & (uint64_t)(1 << PageAddress))) { /* if already locked */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_ALRD_LKD;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    } else {
        EM4233_UserLockBits_Mask |= (uint64_t)(1 << PageAddress); /* write the lock status in mask */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    }

    return ResponseByteCount;
}

uint16_t EM4233_Write_Single(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t* Dataptr;
    uint8_t PageAddress = *FrameInfo.Parameters;
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 5)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (PageAddress > EM4233_NUMBER_OF_BLCKS) {
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount; /* malformed: trying to write in a non-existing block */
    }

    MemoryReadBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + PageAddress), 1);
    Dataptr = PageAddress + 0x01;

    if (LockStatus & ISO15693_MASK_FACTORY_LOCK) {
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway - probably: no factory lock exists? */
    } else if (LockStatus & ISO15693_MASK_USER_LOCK) {
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_CHG_LKD;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    } else {
        MemoryWriteBlock(Dataptr, PageAddress * EM4233_BYTES_PER_BLCK, EM4233_BYTES_PER_BLCK);
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount += 1;
    }

    return ResponseByteCount;
}

uint16_t EM4233_Read_Single(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t FramePtr; /* holds the address where block's data will be put */
    uint8_t PageAddress = *FrameInfo.Parameters;
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (PageAddress >= EM4233_NUMBER_OF_BLCKS) { /* the reader is requesting a sector out of bound */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_NOT_AVL; 
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount;
    }

    FramePtr = 1;

    if (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) { /* request with option flag set */  
        MemoryReadBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + PageAddress), 1);
        if (LockStatus & ISO15693_MASK_FACTORY_LOCK)  { /* tests if the n-th bit of the factory bitmask if set to 1 */
            FrameBuf[FramePtr] = 0x02; /* return bit 1 set as 1 (factory locked) */
        } else if (LockStatus & ISO15693_MASK_USER_LOCK) { /* tests if the n-th bit of the user bitmask if set to 1 */
            FrameBuf[FramePtr] = 0x01; /* return bit 0 set as 1 (user locked) */
        } else
            FrameBuf[FramePtr] = 0x00; /* return lock status 00 (unlocked) */
        FramePtr += 1; /* block's data from byte 2 */
        ResponseByteCount += 1;
    }

    MemoryReadBlock(&FrameBuf[FramePtr], PageAddress * EM4233_BYTES_PER_BLCK, EM4233_BYTES_PER_BLCK);
    ResponseByteCount += 4;

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1;
    
    return ResponseByteCount;
}

uint16_t EM4233_Read_Multiple(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t FramePtr; /* holds the address where block's data will be put */
    uint8_t BlockAddress = FrameInfo.Parameters[0];
    uint8_t BlocksNumber = FrameInfo.Parameters[1] + 0x01; /* according to ISO standard, we have to read 8 blocks if we get 0x07 in request */

    if (FrameInfo.ParamLen != 2)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (BlockAddress > EM4233_NUMBER_OF_BLCKS) { /* the reader is requesting a starting block out of bound */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_NOT_AVL; 
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount;
    } else if ((BlockAddress + BlocksNumber) >= EM4233_NUMBER_OF_BLCKS) { /* last block is out of bound */
        BlocksNumber = EM4233_NUMBER_OF_BLCKS - BlockAddress; /* we read up to latest block, as real tag does */
    }

    FramePtr = 1; /* start of response data  */

    if ( (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) == 0 ) { /* blocks' lock status is not requested */
        /* read data straight into frame */
        MemoryReadBlock(&FrameBuf[FramePtr], BlockAddress * EM4233_BYTES_PER_BLCK, BlocksNumber * EM4233_BYTES_PER_BLCK);
        ResponseByteCount += BlocksNumber * EM4233_BYTES_PER_BLCK;

    } else { /* we have to slice blocks' data with lock statuses */
        uint8_t DataBuffer[ BlocksNumber * EM4233_BYTES_PER_BLCK ]; /* a temporary vector with blocks' content */
        uint8_t LockStatusBuffer[ BlocksNumber ]; /* a vector with blocks' lock status */

        /* read all at once to reduce timing issues */
        MemoryReadBlock(&DataBuffer, BlockAddress * EM4233_BYTES_PER_BLCK, BlocksNumber * EM4233_BYTES_PER_BLCK);
        MemoryReadBlock(&LockStatusBuffer, EM4233_MEM_LSM_ADDRESS + BlockAddress, BlocksNumber);

        for (uint8_t block = 0; block < BlocksNumber; block++) { /* we cycle through the blocks */
            
            /* add lock status */
            if (LockStatusBuffer[block] & ISO15693_MASK_USER_LOCK) { /* tests if bit 0 of the status byte if set to 1 */
                FrameBuf[FramePtr] = 0x01; /* return bit 0 set as 1 (user locked) */
            } else if (LockStatusBuffer[block] & ISO15693_MASK_FACTORY_LOCK)  { /* tests if bit 1 of the status byte if set to 1 */
                FrameBuf[FramePtr] = 0x02; /* return bit 1 set as 1 (factory locked) */
            } else
                FrameBuf[FramePtr] = 0x00; /* return lock status 00 (unlocked) */
            ResponseByteCount += 1;
            FramePtr += 1;

            /* then copy block's data */
            for (uint8_t byte = 0; byte < EM4233_BYTES_PER_BLCK; byte++) { /* we cycle through the bytes in every block */
                FrameBuf[FramePtr] = DataBuffer[block * EM4233_BYTES_PER_BLCK + byte]; /* to copy them in the frame from our temporary buffer */
                FramePtr += 1;
            }
            ResponseByteCount += EM4233_BYTES_PER_BLCK;
        }
    }

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1;
    return ResponseByteCount;
}

uint16_t EM4233_Write_AFI(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t AFI = FrameInfo.Parameters[0];
    uint8_t LockStatus = 0; 

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_AFI_STATUS ) { /* The afi is locked */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }

    MemoryWriteBlock(&AFI, EM4233_MEM_AFI_ADDRESS, 1); /* Actually write new AFI */

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1; 
    return ResponseByteCount;
}

uint16_t EM4233_Lock_AFI(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t LockStatus = 0; 

    if (FrameInfo.ParamLen != 0)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_AFI_STATUS ) { /* The afi is already locked */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }

    LockStatus |= EM4233_MASK_AFI_STATUS;

    MemoryWriteBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1); /* Actually write new AFI */

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1; 
    return ResponseByteCount;
}

uint16_t EM4233_Write_DSFID(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t DSFID = FrameInfo.Parameters[0];
    uint8_t LockStatus = 0; 

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_DSFID_STATUS ) { /* The afi is locked */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }

    MemoryWriteBlock(&DSFID, EM4233_MEM_DSFID_ADDRESS, 1); /* Actually write new AFI */

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1; 
    return ResponseByteCount;
}

uint16_t EM4233_Lock_DSFID(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t LockStatus = 0; 

    if (FrameInfo.ParamLen != 0)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_DSFID_STATUS ) { /* The afi is already locked */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }

    LockStatus |= EM4233_MASK_DSFID_STATUS;

    MemoryWriteBlock(*FrameInfo.Parameters, EM4233_MEM_INF_ADDRESS, 4); /* Actually write the new PASSWORD */

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1; 
    return ResponseByteCount;
}

uint8_t EM4233_Get_SysInfo(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t FramePtr; /* holds the address where block's data will be put */

    if (FrameInfo.ParamLen != 0)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    FramePtr = 1;

    /* I've no idea how this request could generate errors ._.
    if ( ) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }
    */

    /* System info flags */
    FrameBuf[FramePtr] = EM4233_SYSINFO_BYTE; /* check pdf for this */ 
    FramePtr += 1;             /* Move forward the buffer data pointer */
    ResponseByteCount += 1;    /* Increment the response count */

    /* Then append UID */
    uint8_t Uid[ActiveConfiguration.UidSize];
    EM4233GetUid(Uid);
    ISO15693CopyUid(&FrameBuf[FramePtr], Uid);
    FramePtr += ISO15693_GENERIC_UID_SIZE;             /* Move forward the buffer data pointer */
    ResponseByteCount += ISO15693_GENERIC_UID_SIZE;    /* Increment the response count */

    /* Append DSFID */
    if ( EM4233_SYSINFO_BYTE & ( 1 << 0 )) {
        MemoryReadBlock(&FrameBuf[FramePtr], EM4233_MEM_DSFID_ADDRESS, 1);
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    /* Append AFI */
    if ( EM4233_SYSINFO_BYTE & ( 1 << 1 )) {
        MemoryReadBlock(&FrameBuf[FramePtr], EM4233_MEM_AFI_ADDRESS, 1);
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    /* Append VICC memory size */
    if ( EM4233_SYSINFO_BYTE & ( 1 << 2 )) {
        FrameBuf[FramePtr] = EM4233_NUMBER_OF_BLCKS - 0x01;
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */

        FrameBuf[FramePtr] = EM4233_BYTES_PER_BLCK - 0x01;
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    /* Append IC reference */
    if ( EM4233_SYSINFO_BYTE & ( 1 << 3 )) {
        FrameBuf[FramePtr] = EM4233_IC_REFERENCE;
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1; 
    return ResponseByteCount;
}

uint16_t EM4233_Get_Multi_Block_Sec_Stat(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t FramePtr; /* holds the address where block's data will be put */
    uint8_t BlockAddress = FrameInfo.Parameters[0];
    uint8_t BlocksNumber = 0;
    uint8_t LockStatus = 0; 

    if (FrameInfo.ParamLen != 2)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    BlocksNumber = FrameInfo.Parameters[1];

    if (BlockAddress >= EM4233_NUMBER_OF_BLCKS || (BlockAddress + BlocksNumber) >= EM4233_NUMBER_OF_BLCKS) { /* the reader is requesting a sector out of bound */
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_NOT_AVL; /* real TiTag standard reply with this error */
        ResponseByteCount += 2;
        return ResponseByteCount;
    }

    FramePtr = 1; /* Start of response data  */

    for (uint8_t blk = 0; blk <= BlocksNumber; blk++) {

        MemoryReadBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + (BlockAddress + blk)), 1);

        if (LockStatus & ISO15693_MASK_FACTORY_LOCK)  { /* tests if the n-th bit of the factory bitmask if set to 1 */
            FrameBuf[FramePtr] = 0x02; /* return bit 1 set as 1 (factory locked) */
        } else if (LockStatus & ISO15693_MASK_USER_LOCK) { /* tests if the n-th bit of the user bitmask if set to 1 */
            FrameBuf[FramePtr] = 0x01; /* return bit 0 set as 1 (user locked) */
        } else
            FrameBuf[FramePtr] = 0x00; /* return lock status 00 (unlocked) */

        FramePtr += 1;          /* Move forward the buffer data pointer */
        ResponseByteCount += 1; /* Increment the response count */
    }

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1; 
    return ResponseByteCount;
}

uint16_t EM4233_Select(uint8_t* FrameBuf, uint16_t FrameBytes, uint8_t* Uid)
{
    /* I've no idea how this request could generate errors ._.
    if ( ) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }
    */

    bool UidEquals = ISO15693CompareUid(&FrameBuf[ISO15693_REQ_ADDR_PARAM], Uid);

    if (!FrameInfo.Addressed || FrameInfo.Selected) {
        /* tag should remain silent if Select is performed without address flag or with select flag */
        return ISO15693_APP_NO_RESPONSE;
    } else if (State == STATE_SELECTED && !UidEquals) {
        /* tag should remain silent if Select is performed while the tag is selected but against another tag */
        State = STATE_READY;
        return ISO15693_APP_NO_RESPONSE;
    } else if (State != STATE_SELECTED && !UidEquals) {
        /* tag should remain silent if Select is performed against another UID */
        return ISO15693_APP_NO_RESPONSE;
    } else if (State != STATE_SELECTED && UidEquals) {
        State = STATE_SELECTED;
        return ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    }

    return ISO15693_APP_NO_RESPONSE;
}

uint16_t EM4233_Login(uint8_t* FrameBuf, uint16_t FrameBytes, uint8_t* Uid)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t Password[4] = { 0 }; 

    if (FrameInfo.ParamLen != 4)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&Password, EM4233_MEM_PSW_ADDRESS, 4);

    if( false ){ // YES-MAN!
    // if (!memcmp(Password, FrameInfo.Parameters, 4)) { /* Incorrect password */

        loggedIn = false;

        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE;
        return ResponseByteCount;
    }

    loggedIn = true;

    MemoryWriteBlock(Password, EM4233_MEM_PSW_ADDRESS, 4); /* Actually write new AFI */

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1; 
    return ResponseByteCount;
}

uint16_t EM4233AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t Uid[ActiveConfiguration.UidSize];
    EM4233GetUid(Uid);

    if (!ISO15693PrepareFrame(FrameBuf, FrameBytes, &FrameInfo, Uid))
        return ISO15693_APP_NO_RESPONSE;

    if (State == STATE_READY || State == STATE_SELECTED) {
        if (*FrameInfo.Command == ISO15693_CMD_INVENTORY) {
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
            MemoryReadBlock(&FrameBuf[ISO15693_RES_ADDR_PARAM], EM4233_MEM_DSFID_ADDRESS, 1);
            ISO15693CopyUid(&FrameBuf[ISO15693_RES_ADDR_PARAM + 0x01], Uid);
            ResponseByteCount = 10;

        } else if ( (*FrameInfo.Command == ISO15693_CMD_STAY_QUIET ) && FrameInfo.Addressed) {
            State = STATE_QUIET;

        } else if (*FrameInfo.Command == ISO15693_CMD_READ_SINGLE) {
            ResponseByteCount = EM4233_Read_Single(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_WRITE_SINGLE) {
            ResponseByteCount = EM4233_Write_Single(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_LOCK_BLOCK) {
            ResponseByteCount = EM4233_Lock_Block(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_READ_MULTIPLE) {
            ResponseByteCount = EM4233_Read_Multiple(FrameBuf, FrameBytes);
    
        } else if (*FrameInfo.Command == ISO15693_CMD_WRITE_AFI) {
            ResponseByteCount = EM4233_Write_AFI(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_LOCK_AFI) {
            ResponseByteCount = EM4233_Lock_AFI(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_WRITE_DSFID) {
            ResponseByteCount = EM4233_Write_DSFID(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_LOCK_DSFID) {
            ResponseByteCount = EM4233_Lock_DSFID(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_GET_SYS_INFO) {
            ResponseByteCount = EM4233_Get_SysInfo(FrameBuf, FrameBytes);

        } else if (*FrameInfo.Command == ISO15693_CMD_GET_BLOCK_SEC) {
            ResponseByteCount = EM4233_Get_Multi_Block_Sec_Stat(FrameBuf, FrameBytes);
        
        } else if (*FrameInfo.Command == ISO15693_CMD_SELECT) {
            ResponseByteCount = EM4233_Select(FrameBuf, FrameBytes, Uid);

        } else if (*FrameInfo.Command == EM4233_CMD_LOGIN) {
            ResponseByteCount = EM4233_Login(FrameBuf, FrameBytes, Uid);

        } else {
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
            FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_NOT_SUPP;
            ResponseByteCount = 2;
        }
    } else if (State == STATE_QUIET) {
        if (*FrameInfo.Command == ISO15693_CMD_RESET_TO_READY) {
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
            ResponseByteCount = 1;
            State = STATE_READY;

            FrameInfo.Flags         = NULL;
            FrameInfo.Command       = NULL;
            FrameInfo.Parameters    = NULL;
            FrameInfo.ParamLen      = 0;
            FrameInfo.Addressed     = false;
            FrameInfo.Selected      = false;
        }
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