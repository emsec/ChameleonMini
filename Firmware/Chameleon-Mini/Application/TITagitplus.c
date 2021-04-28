/*
 * TITagitplus.c
 *
 *  Created 20210409
 *      Author: leandre84
 *      Based on TI Tag-it Standard implementation, read multiple inspired by Sl2s2002 implementation
 */

#ifdef CONFIG_TITAGITPLUS_SUPPORT

#include "ISO15693-A.h"
#include "TITagitplus.h"

static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

uint64_t UserLockBits_Mask_Tagitplus = 0;     /* Holds lock state of blocks 1-64 */
uint64_t FactoryLockBits_Mask_Tagitplus = 0;  /* Holds lock state of blocks 1-64 */

void TITagitplusAppInit(void) {
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;

    MemoryReadBlock(&MyAFI, TITAGIT_PLUS_MEM_AFI_ADDRESS, 1);
    TITagitplusGetUid(Uid);
}

void TITagitplusAppReset(void) {
    State = STATE_READY;

    FrameInfo.Flags         = NULL;
    FrameInfo.Command       = NULL;
    FrameInfo.Parameters    = NULL;
    FrameInfo.ParamLen      = 0;
    FrameInfo.Addressed     = false;
    FrameInfo.Selected      = false;

    MemoryReadBlock(&MyAFI, TITAGIT_PLUS_MEM_AFI_ADDRESS, 1);
    TITagitplusGetUid(Uid);
}


void TITagitplusAppTask(void) {

}

void TITagitplusAppTick(void) {

}

uint16_t TITagitplusAppProcess(uint8_t *FrameBuf, uint16_t FrameBytes) {
    ResponseByteCount = ISO15693_APP_NO_RESPONSE;

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

            } else if (*FrameInfo.Command == ISO15693_CMD_READ_SINGLE || *FrameInfo.Command == ISO15693_CMD_READ_MULTIPLE) {
                uint8_t *FramePtr; /* holds the address where block's data will be put */
                uint8_t PageAddress = *FrameInfo.Parameters;
                uint8_t PageAddressCount = 1;

                if (*FrameInfo.Command == ISO15693_CMD_READ_SINGLE) {
                    if (FrameInfo.ParamLen != 1) {
                        break; /* malformed: not enough or too much data */
                    }       
                } else {
                    if (FrameInfo.ParamLen != 2) {
                        break; /* malformed: not enough or too much data */
                    }
                    PageAddressCount = *(FrameInfo.Parameters+1)+1;
                }

                if (PageAddress >= TITAGIT_PLUS_NUMBER_OF_USER_SECTORS) { /* the reader is requesting a sector out of bound */
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_NOT_AVL; /* real TiTag standard reply with this error */
                    ResponseByteCount = 2;
                    break;
                }

                /* position to one after flag bit and keep its value for now as we still need it to determine if option bit is set */
                FramePtr = FrameBuf + 1;
                ResponseByteCount = 1;

                for (int i = 0; i < PageAddressCount; i++) {
                    /* write additional bit with locking information */
                    if (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_OPTION) { /* request with option flag set - handle extra byte in response*/
                        if (FactoryLockBits_Mask_Tagitplus & (1 << PageAddress)) { /* tests if the n-th bit of the factory bitmask if set to 1 */
                            *(FramePtr) = 0x02; /* return bit 1 set as 1 (factory locked) */
                        } else if (UserLockBits_Mask_Tagitplus & (1 << PageAddress)) { /* tests if the n-th bit of the user bitmask if set to 1 */
                            *(FramePtr) = 0x01; /* return bit 0 set as 1 (user locked) */
                        } else {
                            *(FramePtr) = 0x00; /* return lock status 00 (unlocked) */
                        }      

                        FramePtr++;
                        ResponseByteCount++;
                    }

                    /* write data page, increment response bytes and prepare pointer etc for next iteration */
                    MemoryReadBlock(FramePtr, PageAddress * TITAGIT_PLUS_BYTES_PER_PAGE, TITAGIT_PLUS_BYTES_PER_PAGE);
                    FramePtr += TITAGIT_PLUS_BYTES_PER_PAGE;
                    ResponseByteCount += TITAGIT_PLUS_BYTES_PER_PAGE;
                    PageAddress++;
                }

                FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR; /* flags */

            } else if (*FrameInfo.Command == ISO15693_CMD_WRITE_SINGLE) {
                uint8_t *Dataptr;
                uint8_t PageAddress = *FrameInfo.Parameters;

                if (FrameInfo.ParamLen != 5)
                    break; /* malformed: not enough or too much data */

                if (PageAddress > TITAGIT_PLUS_NUMBER_OF_USER_SECTORS) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
                    ResponseByteCount = 2;
                    break; /* malformed: trying to write in a non-existing block */
                }

                Dataptr = FrameInfo.Parameters + 1;

                if (FactoryLockBits_Mask_Tagitplus & (1 << PageAddress)) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
                    ResponseByteCount = 2;
                } else if (UserLockBits_Mask_Tagitplus & (1 << PageAddress)) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_CHG_LKD;
                    ResponseByteCount = 2;
                } else {
                    MemoryWriteBlock(Dataptr, PageAddress * TITAGIT_PLUS_BYTES_PER_PAGE, TITAGIT_PLUS_BYTES_PER_PAGE);
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                    ResponseByteCount = 1;
                }

            } else if (*FrameInfo.Command == ISO15693_CMD_LOCK_BLOCK) {
                uint8_t PageAddress = *FrameInfo.Parameters;

                if (FrameInfo.ParamLen != 1)
                    break; /* malformed: not enough or too much data */

                if (PageAddress > TITAGIT_PLUS_NUMBER_OF_SECTORS) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_OPT_NOT_SUPP;
                    ResponseByteCount = 2;
                    break; /* malformed: trying to lock a non-existing block */
                }

                if ((FactoryLockBits_Mask_Tagitplus & (1 << PageAddress)) || (UserLockBits_Mask_Tagitplus & (1 << PageAddress))) {
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_ERROR;
                    FrameBuf[ISO15693_RES_ADDR_PARAM] = ISO15693_RES_ERR_BLK_ALRD_LKD;
                    ResponseByteCount = 2;
                } else {
                    UserLockBits_Mask_Tagitplus |= (1 << PageAddress); /*  */
                    FrameBuf[ISO15693_ADDR_FLAGS] = ISO15693_RES_FLAG_NO_ERROR;
                    ResponseByteCount = 1;
                }

            } else if (*FrameInfo.Command == ISO15693_CMD_GET_SYS_INFO) {
                FrameBuf[0] = 0; /* Flags */
                FrameBuf[1] = 0x0F; /* InfoFlags */
                ISO15693CopyUid(&FrameBuf[2], Uid);
                FrameBuf[10] = 0x00;
                FrameBuf[11] = 0x00;
                FrameBuf[12] = 0x3F;
                FrameBuf[13] = 0x03;
                FrameBuf[14] = 0x8B;
                ResponseByteCount = 15;
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

    return ResponseByteCount;
}

void TITagitplusGetUid(ConfigurationUidType Uid) {
    MemoryReadBlock(&Uid[0], TITAGIT_PLUS_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);

    // Reverse UID after reading it
    TITagitplusFlipUid(Uid);
}

void TITagitplusSetUid(ConfigurationUidType NewUid) {
    memcpy(Uid, NewUid, ActiveConfiguration.UidSize); // Update the local variable
    // Reverse UID before writing it
    TITagitplusFlipUid(Uid);

    MemoryWriteBlock(Uid, TITAGIT_PLUS_MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void TITagitplusFlipUid(ConfigurationUidType Uid) {
    uint8_t tmp, *tail;
    tail = Uid + ActiveConfiguration.UidSize - 1;
    while (Uid < tail) {
        tmp = *Uid;
        *Uid++ = *tail;
        *tail-- = tmp;
    }
}

#endif /* CONFIG_TITAGITPLUS_SUPPORT */
