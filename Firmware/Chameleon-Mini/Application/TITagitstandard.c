/*
 * TITagitstandard.c
 *
 *  Created on: 01-03-2017
 *      Author: Phillip Nash
 *      Modified by rickventura for texas 15693 tag-it STANDARD
 *      Modified by ceres-c & MrMoDDoM to finish things up
 */

#include "ISO15693-A.h"
#include "TITagitstandard.h"

static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

uint16_t UserLockBits_Mask = 0;     /* Holds lock state of blocks */
uint16_t FactoryLockBits_Mask = 0;  /* Holds lock state of blocks */

void TITagitstandardAppInit(void) {
    State = STATE_READY;

    FactoryLockBits_Mask |= (1 << 8);   /* Locks block 8... */
    FactoryLockBits_Mask |= (1 << 9);   /* ...and 9, which contains the UID */

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;

    MemoryReadBlock(&MyAFI, TITAGIT_MEM_AFI_ADDRESS, 1);
}

void TITagitstandardAppReset(void) {
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;
}


void TITagitstandardAppTask(void) {

}

void TITagitstandardAppTick(void) {

}

uint16_t TITagitstandardAppProcess(uint8_t *FrameBuf, uint16_t FrameBytes) {
    uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
    uint8_t Uid[ActiveConfiguration.UidSize];
    TITagitstandardGetUid(Uid);

    if ((FrameBytes < ISO15693_MIN_FRAME_SIZE) || !ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE))
        /* malformed frame */
        return ResponseByteCount;

    if (!ISO15693PrepareFrame(FrameBuf, FrameBytes, &FrameInfo, State == STATE_SELECTED, Uid, MyAFI))
        return ISO15693_APP_NO_RESPONSE;

    switch (State) {
        case STATE_READY:
            if (*FrameInfo.Command == ISO15693_CMD_INVENTORY) {
                FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_INVENTORY_DSFID;
                ISO15693CopyUid(&FrameBuf[ISO15693_RES_ADDR_PARAM + 0x01], Uid);
                ResponseByteCount = 10;

            } else if (*FrameInfo.Command == ISO15693_CMD_STAY_QUIET && FrameInfo.Addressed) {
                State = STATE_QUIET;

            } else if (*FrameInfo.Command == ISO15693_CMD_READ_SINGLE) {
                uint8_t *FramePtr; /* holds the address where block's data will be put */
                uint8_t PageAddress = *FrameInfo.Parameters;

                if (FrameInfo.ParamLen != 1)
                    break; /* malformed: not enough or too much data */

                if (PageAddress >= TITAGIT_NUMBER_OF_SECTORS) { /* the reader is requesting a sector out of bound */
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_NOT_AVL; /* real TiTag standard reply with this error */
                    ResponseByteCount = 2;
                    break;
                }

                if (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) { /* request with option flag set */
                    if (FactoryLockBits_Mask & (1 << PageAddress)) { /* tests if the n-th bit of the factory bitmask if set to 1 */
                        FrameBuf[1] = 0x02; /* return bit 1 set as 1 (factory locked) */
                    } else if (UserLockBits_Mask & (1 << PageAddress)) { /* tests if the n-th bit of the user bitmask if set to 1 */
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
                MemoryReadBlock(FramePtr, PageAddress * TITAGIT_BYTES_PER_PAGE, TITAGIT_BYTES_PER_PAGE);

            } else if (*FrameInfo.Command == ISO15693_CMD_WRITE_SINGLE) {
                uint8_t *Dataptr;
                uint8_t PageAddress = *FrameInfo.Parameters;

                if (FrameInfo.ParamLen != 5)
                    break; /* malformed: not enough or too much data */

                if (PageAddress > TITAGIT_NUMBER_OF_SECTORS) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
                    ResponseByteCount = 2;
                    break; /* malformed: trying to write in a non-existing block */
                }

                Dataptr = FrameInfo.Parameters + 1;

                if (FactoryLockBits_Mask & (1 << PageAddress)) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
                    ResponseByteCount = 2;
                } else if (UserLockBits_Mask & (1 << PageAddress)) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_CHG_LKD;
                    ResponseByteCount = 2;
                } else {
                    MemoryWriteBlock(Dataptr, PageAddress * TITAGIT_BYTES_PER_PAGE, TITAGIT_BYTES_PER_PAGE);
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                    ResponseByteCount = 1;
                }

            } else if (*FrameInfo.Command == ISO15693_CMD_LOCK_BLOCK) {
                uint8_t PageAddress = *FrameInfo.Parameters;

                if (FrameInfo.ParamLen != 1)
                    break; /* malformed: not enough or too much data */

                if (PageAddress > TITAGIT_NUMBER_OF_SECTORS) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
                    ResponseByteCount = 2;
                    break; /* malformed: trying to lock a non-existing block */
                }

                if ((FactoryLockBits_Mask & (1 << PageAddress)) || (UserLockBits_Mask & (1 << PageAddress))) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_ALRD_LKD;
                    ResponseByteCount = 2;
                } else {
                    UserLockBits_Mask |= (1 << PageAddress); /*  */
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                    ResponseByteCount = 1;
                }

            } else {
                FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_NOT_SUPP;
                ResponseByteCount = 2;
            }
            break;

        case STATE_SELECTED:
            /* Selected state is not supported by Ti TagIt Standard */
            break;

        case STATE_QUIET:
            /* Ti TagIt Standard does not support reset to ready command */
            /* following code is lef for future reference for other tags */
            /*
            if (Command == ISO15693_CMD_RESET_TO_READY) {
                FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                ResponseByteCount = 1;
                State = STATE_READY;

                FrameInfo.Flags         = NULL;
                FrameInfo.Command       = NULL;
                FrameInfo.Parameters    = NULL;
                FrameInfo.ParamLen      = 0;
                FrameInfo.Addressed     = false;
            }
            */
            ResponseByteCount = 0; /* better safe than sorry */
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

void TITagitstandardGetUid(ConfigurationUidType Uid) {
    MemoryReadBlock(&Uid[0], TITAGIT_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);

    // Reverse UID after reading it
    TITagitstandardFlipUid(Uid);
}

void TITagitstandardSetUid(ConfigurationUidType Uid) {
    // Reverse UID before writing it
    TITagitstandardFlipUid(Uid);

    MemoryWriteBlock(Uid, TITAGIT_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void TITagitstandardFlipUid(ConfigurationUidType Uid) {
    uint8_t tmp, *tail;
    tail = Uid + ActiveConfiguration.UidSize - 1;
    while (Uid < tail) {
        tmp = *Uid;
        *Uid++ = *tail;
        *tail-- = tmp;
    }
}
