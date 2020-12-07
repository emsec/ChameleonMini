/*
 * EM4233.c
 *
 *  Created on: 12-05-2018
 *      Author: ceres-c & MrMoDDoM
 *  Notes:
 *      - In EM4233.h you can find the define EM4233_LOGIN_YES_CARD that has to be uncommented
 *        to allow any login request without checking the password.
 *
 *  TODO:
 *      - Check with real tag every command's actual response in addressed/selected State
 *          (Only EM4233_Read_Single and EM4233_Read_Multiple have been checked up to now)
 */

#include "../Random.h"
#include "ISO15693-A.h"
#include "EM4233.h"

static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

bool loggedIn;

void EM4233AppInit(void) {
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;
    loggedIn = false;
    MemoryReadBlock(&MyAFI, EM4233_MEM_AFI_ADDRESS, 1);
    MemoryReadBlock(&Uid, EM4233_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void EM4233AppReset(void) {
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;
    loggedIn = false;
    MemoryReadBlock(&MyAFI, EM4233_MEM_AFI_ADDRESS, 1);
    MemoryReadBlock(&Uid, EM4233_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void EM4233AppTask(void) {

}

void EM4233AppTick(void) {

}

uint16_t EM4233_Lock_Block(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t BlockAddress = *FrameInfo.Parameters;
    uint8_t LockStatus = 0;

    MemoryReadBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + BlockAddress), 1);

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (BlockAddress > EM4233_NUMBER_OF_BLCKS) {
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount; /* malformed: trying to lock a non-existing block */
    }


    if (LockStatus > ISO15693_MASK_UNLOCKED) { /* LockStatus 0x00 represent unlocked block, greater values are different kind of locks */
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    } else {
        LockStatus |= ISO15693_MASK_USER_LOCK;
        MemoryWriteBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + BlockAddress), 1); /* write user lock in memory */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    }

    return ResponseByteCount;
}

uint16_t EM4233_Write_Single(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t BlockAddress = *FrameInfo.Parameters;
    uint8_t *Dataptr = FrameInfo.Parameters + 0x01; /* Data to write begins on 2nd byte of the frame received by the reader */
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 5)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (BlockAddress > EM4233_NUMBER_OF_BLCKS) {
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount; /* malformed: trying to write in a non-existing block */
    }

    MemoryReadBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + BlockAddress), 1);

    if (LockStatus & ISO15693_MASK_FACTORY_LOCK) {
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway - probably: no factory lock exists? */
    } else if (LockStatus & ISO15693_MASK_USER_LOCK) {
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_CHG_LKD;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    } else {
        MemoryWriteBlock(Dataptr, BlockAddress * EM4233_BYTES_PER_BLCK, EM4233_BYTES_PER_BLCK);
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount += 1;
    }

    return ResponseByteCount;
}

uint16_t EM4233_Read_Single(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t FramePtr; /* holds the address where block's data will be put */
    uint8_t BlockAddress = FrameInfo.Parameters[0];
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (BlockAddress >= EM4233_NUMBER_OF_BLCKS) { /* check if the reader is requesting a sector out of bound */
        if (FrameInfo.Addressed) { /* If the request is addressed */
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
            FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
            ResponseByteCount += 2; /* Copied this behaviour from real tag, not specified in ISO documents */
        }
        return ResponseByteCount; /* If not addressed real tag does not respond */
    }

    FramePtr = 1;

    if (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) { /* request with option flag set */
        MemoryReadBlock(&LockStatus, (EM4233_MEM_LSM_ADDRESS + BlockAddress), 1);
        if (LockStatus & ISO15693_MASK_FACTORY_LOCK)  { /* tests if the n-th bit of the factory bitmask if set to 1 */
            FrameBuf[FramePtr] = ISO15693_MASK_FACTORY_LOCK; /* return bit 1 set as 1 (factory locked) */
        } else if (LockStatus & ISO15693_MASK_USER_LOCK) { /* tests if the n-th bit of the user bitmask if set to 1 */
            FrameBuf[FramePtr] = ISO15693_MASK_USER_LOCK; /* return bit 0 set as 1 (user locked) */
        } else
            FrameBuf[FramePtr] = ISO15693_MASK_UNLOCKED; /* return lock status 00 (unlocked) */
        FramePtr += 1; /* block's data from byte 2 */
        ResponseByteCount += 1;
    }

    MemoryReadBlock(&FrameBuf[FramePtr], BlockAddress * EM4233_BYTES_PER_BLCK, EM4233_BYTES_PER_BLCK);
    ResponseByteCount += 4;

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1;

    return ResponseByteCount;
}

uint16_t EM4233_Read_Multiple(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t FramePtr; /* holds the address where block's data will be put */
    uint8_t BlockAddress = FrameInfo.Parameters[0];
    uint8_t BlocksNumber = FrameInfo.Parameters[1] + 0x01; /* according to ISO standard, we have to read 0x08 blocks if we get 0x07 in request */

    if (FrameInfo.ParamLen != 2)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    if (BlockAddress >= EM4233_NUMBER_OF_BLCKS) { /* the reader is requesting a block out of bound */
        if (FrameInfo.Addressed) { /* If the request is addressed */
            FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
            FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
            ResponseByteCount += 2; /* Copied this behaviour from real tag, not specified in ISO documents */
        }
        return ResponseByteCount; /* If not addressed real tag does not respond */
    } else if ((BlockAddress + BlocksNumber) >= EM4233_NUMBER_OF_BLCKS) { /* last block is out of bound */
        BlocksNumber = EM4233_NUMBER_OF_BLCKS - BlockAddress; /* we read up to latest block, as real tag does */
    }

    FramePtr = 1; /* start of response data  */

    if ((FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) == 0) {   /* blocks' lock status is not requested */
        /* read data straight into frame */
        MemoryReadBlock(&FrameBuf[FramePtr], BlockAddress * EM4233_BYTES_PER_BLCK, BlocksNumber * EM4233_BYTES_PER_BLCK);
        ResponseByteCount += BlocksNumber * EM4233_BYTES_PER_BLCK;

    } else { /* we have to slice blocks' data with lock statuses */
        uint8_t DataBuffer[ BlocksNumber * EM4233_BYTES_PER_BLCK ]; /* a temporary vector with blocks' content */
        uint8_t LockStatusBuffer[ BlocksNumber ]; /* a temporary vector with blocks' lock status */

        /* read all at once to reduce timing issues */
        MemoryReadBlock(&DataBuffer, BlockAddress * EM4233_BYTES_PER_BLCK, BlocksNumber * EM4233_BYTES_PER_BLCK);
        MemoryReadBlock(&LockStatusBuffer, EM4233_MEM_LSM_ADDRESS + BlockAddress, BlocksNumber);

        for (uint8_t block = 0; block < BlocksNumber; block++) { /* we cycle through the blocks */

            /* add lock status */
            FrameBuf[FramePtr++] = LockStatusBuffer[block]; /* Byte in dump equals to the byte that has to be sent */
            /* I.E. We store 0x01 to identify user lock, which is the same as what ISO15693 enforce */
            ResponseByteCount += 1;

            /* then copy block's data */
            FrameBuf[FramePtr++] = DataBuffer[block * EM4233_BYTES_PER_BLCK + 0];
            FrameBuf[FramePtr++] = DataBuffer[block * EM4233_BYTES_PER_BLCK + 1];
            FrameBuf[FramePtr++] = DataBuffer[block * EM4233_BYTES_PER_BLCK + 2];
            FrameBuf[FramePtr++] = DataBuffer[block * EM4233_BYTES_PER_BLCK + 3];
            ResponseByteCount += EM4233_BYTES_PER_BLCK;
        }
    }

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1;
    return ResponseByteCount;
}

uint16_t EM4233_Write_AFI(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t AFI = FrameInfo.Parameters[0];
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_AFI_STATUS) {  /* The AFI is locked */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        // ResponseByteCount += 2;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount;
    }

    MemoryWriteBlock(&AFI, EM4233_MEM_AFI_ADDRESS, 1); /* Actually write new AFI */
    MyAFI = AFI; /* And update global variable */

    // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    // ResponseByteCount += 1;
    ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    return ResponseByteCount;
}

uint16_t EM4233_Lock_AFI(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 0)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_AFI_STATUS) {  /* The AFI is already locked */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        // ResponseByteCount += 2;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount;
    }

    LockStatus |= EM4233_MASK_AFI_STATUS;

    MemoryWriteBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1); /* Write in info bits AFI lockdown */

    // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    // ResponseByteCount += 1;
    ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    return ResponseByteCount;
}

uint16_t EM4233_Write_DSFID(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t DSFID = FrameInfo.Parameters[0];
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 1)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_DSFID_STATUS) {  /* The DSFID is locked */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        // ResponseByteCount += 2;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount;
    }

    MemoryWriteBlock(&DSFID, EM4233_MEM_DSFID_ADDRESS, 1); /* Actually write new DSFID */

    // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    // ResponseByteCount += 1;
    ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    return ResponseByteCount;
}

uint16_t EM4233_Lock_DSFID(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t LockStatus = 0;

    if (FrameInfo.ParamLen != 0)
        return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

    MemoryReadBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1);

    if (LockStatus & EM4233_MASK_DSFID_STATUS) {  /* The DSFID is already locked */
        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        // ResponseByteCount += 2;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
        return ResponseByteCount;
    }

    LockStatus |= EM4233_MASK_DSFID_STATUS;

    MemoryWriteBlock(&LockStatus, EM4233_MEM_INF_ADDRESS, 1); /* Write in info bits DSFID lockdown */

    // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    // ResponseByteCount += 1;
    ResponseByteCount = ISO15693_APP_NO_RESPONSE; /* real tag does not respond anyway */
    return ResponseByteCount;
}

uint8_t EM4233_Get_SysInfo(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
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
    FrameBuf[FramePtr] = EM4233_SYSINFO_BYTE; /* check EM4233SLIC datasheet for this */
    FramePtr += 1;             /* Move forward the buffer data pointer */
    ResponseByteCount += 1;    /* Increment the response count */

    /* Then append UID */
    ISO15693CopyUid(&FrameBuf[FramePtr], Uid);
    FramePtr += ISO15693_GENERIC_UID_SIZE;             /* Move forward the buffer data pointer */
    ResponseByteCount += ISO15693_GENERIC_UID_SIZE;    /* Increment the response count */

    /* Append DSFID */
    if (EM4233_SYSINFO_BYTE & (1 << 0)) {
        MemoryReadBlock(&FrameBuf[FramePtr], EM4233_MEM_DSFID_ADDRESS, 1);
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    /* Append AFI */
    if (EM4233_SYSINFO_BYTE & (1 << 1)) {
        MemoryReadBlock(&FrameBuf[FramePtr], EM4233_MEM_AFI_ADDRESS, 1);
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    /* Append VICC memory size */
    if (EM4233_SYSINFO_BYTE & (1 << 2)) {
        FrameBuf[FramePtr] = EM4233_NUMBER_OF_BLCKS - 0x01;
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */

        FrameBuf[FramePtr] = EM4233_BYTES_PER_BLCK - 0x01;
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    /* Append IC reference */
    if (EM4233_SYSINFO_BYTE & (1 << 3)) {
        FrameBuf[FramePtr] = EM4233_IC_REFERENCE;
        FramePtr += 1;             /* Move forward the buffer data pointer */
        ResponseByteCount += 1;    /* Increment the response count */
    }

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1;
    return ResponseByteCount;
}

uint16_t EM4233_Get_Multi_Block_Sec_Stat(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t FramePtr; /* holds the address where block's data will be put */
    uint8_t BlockAddress = FrameInfo.Parameters[0];
    uint8_t BlocksNumber = FrameInfo.Parameters[1] + 0x01;

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

    /* read all at once to reduce timing issues */
    MemoryReadBlock(&FrameBuf[FramePtr], EM4233_MEM_LSM_ADDRESS + BlockAddress, BlocksNumber);
    ResponseByteCount += BlocksNumber;

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1;
    return ResponseByteCount;
}

uint16_t EM4233_Select(uint8_t *FrameBuf, uint16_t FrameBytes, uint8_t *Uid) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    /* I've no idea how this request could generate errors ._.
    if ( ) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }
    */

    bool UidEquals = ISO15693CompareUid(&FrameBuf[ISO15693_REQ_ADDR_PARAM], Uid);

    if (!(FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_ADDRESS) ||
            (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_SELECT)
       ) {
        /* tag should remain silent if Select is performed without address flag or with select flag */
        return ISO15693_APP_NO_RESPONSE;
    } else if (!UidEquals) {
        /* tag should remain silent and reset if Select is performed against another UID,
         * whether our the tag is selected or not
         */
        State = STATE_READY;
        return ISO15693_APP_NO_RESPONSE;
    } else if (State != STATE_SELECTED && UidEquals) {
        State = STATE_SELECTED;
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
        ResponseByteCount += 1;
        return ResponseByteCount;
    }

    /* This should never happen (TM), I've added it to shut the compiler up */
    State = STATE_READY;
    return ISO15693_APP_NO_RESPONSE;
}

uint16_t EM4233_Reset_To_Ready(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    /* I've no idea how this request could generate errors ._.
    if ( ) {
        FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount += 2;
        return ResponseByteCount;
    }
    */
    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;

    State = STATE_READY;

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
    ResponseByteCount += 1;
    return ResponseByteCount;
}

uint16_t EM4233_Login(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t Password[4] = { 0 };

    if (FrameInfo.ParamLen != 4 || !FrameInfo.Addressed || !(FrameInfo.Selected && State == STATE_SELECTED))
        /* Malformed: not enough or too much data. Also this command only works in addressed mode */
        return ISO15693_APP_NO_RESPONSE;

    MemoryReadBlock(&Password, EM4233_MEM_PSW_ADDRESS, 4);

#ifdef EM4233_LOGIN_YES_CARD
    /* Accept any password from reader as correct one */
    loggedIn = true;

    MemoryWriteBlock(FrameInfo.Parameters, EM4233_MEM_PSW_ADDRESS, 4); /* Store password in memory for retrival */

#else
    /* Check if the password is actually the right one */
    if (memcmp(Password, FrameInfo.Parameters, 4) != 0) { /* Incorrect password */
        loggedIn = false;

        // FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
        // FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_GENERIC;
        ResponseByteCount = ISO15693_APP_NO_RESPONSE;
        return ResponseByteCount;
    }

#endif

    loggedIn = true;

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */
    ResponseByteCount += 1;
    return ResponseByteCount;
}

uint16_t EM4233_Auth1(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    // uint8_t KeyNo = *FrameInfo.Parameters; /* Right now this parameter is unused, but it will be useful */

    if (FrameInfo.ParamLen != 1) /* Malformed: not enough or too much data */
        return ISO15693_APP_NO_RESPONSE;

    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
#ifdef EM4233_LOGIN_YES_CARD
    /* Will be useful for testing purposes, probably removed in final version */
    FrameBuf[ISO15693_ADDR_FLAGS + 1] = 0x00;
    FrameBuf[ISO15693_ADDR_FLAGS + 2] = 0x00;
    FrameBuf[ISO15693_ADDR_FLAGS + 3] = 0x00;
    FrameBuf[ISO15693_ADDR_FLAGS + 4] = 0x00;
    FrameBuf[ISO15693_ADDR_FLAGS + 5] = 0x00;
    FrameBuf[ISO15693_ADDR_FLAGS + 6] = 0x00;
    FrameBuf[ISO15693_ADDR_FLAGS + 7] = 0x00;
#else
    /* Respond like a real tag */
    RandomGetBuffer(FrameBuf + 0x01, 7); /* Random number A1 in EM Marin definitions */
#endif
    ResponseByteCount += 8;
    return ResponseByteCount;
}

uint16_t EM4233_Auth2(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    // uint8_t A2 = FrameInfo.Parameters;
    // uint8_t f = FrameInfo.Parameters + 0x08;
    // uint8_t g[3] = { 0 }; /* Names according to EM Marin definitions */

    if (FrameInfo.ParamLen != 11) /* Malformed: not enough or too much data */
        return ISO15693_APP_NO_RESPONSE;
    // else if (!fFunc()) /* Actual f implementation to check if the f bytes we received are correct */
    //     return ISO15693_APP_NO_RESPONSE;


    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
    RandomGetBuffer(FrameBuf + 0x01, 3); /* This should be replaced with actual gFunc() implementation */
    ResponseByteCount += 4;
    return ResponseByteCount;
}

uint16_t EM4233AppProcess(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;

    if ((FrameBytes < ISO15693_MIN_FRAME_SIZE) || !ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE))
        /* malformed frame */
        return ResponseByteCount;

    if (FrameBuf[ISO15693_REQ_ADDR_CMD] == ISO15693_CMD_SELECT) {
        /* Select has its own path before PrepareFrame because we have to change the variable State
         * from Select to Ready if "Select" cmd is addressed to another tag.
         * It felt weird to add this kind of check in ISO15693PrepareFrame, which should not
         * interfere with tag specific variables, such as State in this case.
         */
        ResponseByteCount = EM4233_Select(FrameBuf, FrameBytes, Uid);
    } else if (!ISO15693PrepareFrame(FrameBuf, FrameBytes, &FrameInfo, State == STATE_SELECTED, Uid, MyAFI))
        return ISO15693_APP_NO_RESPONSE;

    if (State == STATE_READY || State == STATE_SELECTED) {
        switch (*FrameInfo.Command) {
            case ISO15693_CMD_INVENTORY:
                if (FrameInfo.ParamLen == 0)
                    return ISO15693_APP_NO_RESPONSE; /* malformed: not enough or too much data */

                if (ISO15693AntiColl(FrameBuf, FrameBytes, &FrameInfo, Uid)) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                    MemoryReadBlock(&FrameBuf[ISO15693_RES_ADDR_PARAM], EM4233_MEM_DSFID_ADDRESS, 1);
                    ISO15693CopyUid(&FrameBuf[ISO15693_RES_ADDR_PARAM + 0x01], Uid);
                    ResponseByteCount += 10;
                }
                break;

            case ISO15693_CMD_STAY_QUIET:
                if (FrameInfo.Addressed)
                    State = STATE_QUIET;
                break;

            case ISO15693_CMD_READ_SINGLE:
                ResponseByteCount = EM4233_Read_Single(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_WRITE_SINGLE:
                ResponseByteCount = EM4233_Write_Single(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_LOCK_BLOCK:
                ResponseByteCount = EM4233_Lock_Block(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_READ_MULTIPLE:
                ResponseByteCount = EM4233_Read_Multiple(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_WRITE_AFI:
                ResponseByteCount = EM4233_Write_AFI(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_LOCK_AFI:
                ResponseByteCount = EM4233_Lock_AFI(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_WRITE_DSFID:
                ResponseByteCount = EM4233_Write_DSFID(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_LOCK_DSFID:
                ResponseByteCount = EM4233_Lock_DSFID(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_GET_SYS_INFO:
                ResponseByteCount = EM4233_Get_SysInfo(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_GET_BLOCK_SEC:
                ResponseByteCount = EM4233_Get_Multi_Block_Sec_Stat(FrameBuf, FrameBytes);
                break;

            case ISO15693_CMD_RESET_TO_READY:
                ResponseByteCount = EM4233_Reset_To_Ready(FrameBuf, FrameBytes);
                break;

            case EM4233_CMD_LOGIN:
                ResponseByteCount = EM4233_Login(FrameBuf, FrameBytes);
                break;

            case EM4233_CMD_AUTH1:
                ResponseByteCount = EM4233_Auth1(FrameBuf, FrameBytes);
                break;

            case EM4233_CMD_AUTH2:
                ResponseByteCount = EM4233_Auth2(FrameBuf, FrameBytes);
                break;

            default:
                if (FrameInfo.Addressed) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_NOT_SUPP;
                    ResponseByteCount = 2;
                } /* EM4233 respond with error flag only to addressed commands */
                break;
        }

    } else if (State == STATE_QUIET) {
        if (*FrameInfo.Command == ISO15693_CMD_RESET_TO_READY) {
            ResponseByteCount = EM4233_Reset_To_Ready(FrameBuf, FrameBytes);
        }
    }

    return ResponseByteCount;
}

void EM4233GetUid(ConfigurationUidType Uid) {
    MemoryReadBlock(&Uid[0], EM4233_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void EM4233SetUid(ConfigurationUidType NewUid) {
    memcpy(Uid, NewUid, ActiveConfiguration.UidSize);
    MemoryWriteBlock(NewUid, EM4233_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}
