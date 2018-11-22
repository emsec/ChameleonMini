/*
 * ISO15693-3.h
 *
 *  Created on: 24.08.2013
 *      Author: skuser
 */

#ifndef ISO15693_3_H_
#define ISO15693_3_H_

#include "../Common.h"

/* request and response fields addresses */
#define ISO15693_ADDR_FLAGS             0x00
#define ISO15693_REQ_ADDR_CMD           0x01
#define ISO15693_REQ_ADDR_PARAM         0x02

#define ISO15693_RES_ADDR_PARAM         0x01

/* command codes */
#define ISO15693_CMD_INVENTORY          0x01
#define ISO15693_CMD_STAY_QUIET         0x02
#define ISO15693_CMD_READ_SINGLE        0x20
#define ISO15693_CMD_WRITE_SINGLE       0x21
#define ISO15693_CMD_LOCK_BLOCK         0x22
#define ISO15693_CMD_READ_MULTIPLE      0x23
#define ISO15693_CMD_WRITE_MULTIPLE     0x24
#define ISO15693_CMD_SELECT             0x25
#define ISO15693_CMD_RESET_TO_READY     0x26
#define ISO15693_CMD_WRITE_AFI          0x27
#define ISO15693_CMD_LOCK_AFI           0x28
#define ISO15693_CMD_WRITE_DSFID        0x29
#define ISO15693_CMD_LOCK_DSFID         0x2A
#define ISO15693_CMD_GET_SYS_INFO       0x2B
#define ISO15693_CMD_GET_BLOCK_SEC      0x2C

#define ISO15693_REQ_FLAG_SUBCARRIER    0x01
#define ISO15693_REQ_FLAG_DATARATE      0x02
#define ISO15693_REQ_FLAG_INVENTORY     0x04
#define ISO15693_REQ_FLAG_PROT_EXT      0x08
#define ISO15693_REQ_FLAG_OPTION        0x40
#define ISO15693_REQ_FLAG_RFU           0x80
/* When INVENTORY flag is not set: */
#define ISO15693_REQ_FLAG_SELECT        0x10
#define ISO15693_REQ_FLAG_ADDRESS       0x20
/* When INVENTORY flag is set: */
#define ISO15693_REQ_FLAG_AFI           0x10
#define ISO15693_REQ_FLAG_NB_SLOTS      0x20

#define ISO15693_RES_FLAG_NO_ERROR      0x00
#define ISO15693_RES_FLAG_ERROR         0x01
#define ISO15693_RES_FLAG_PROT_EXT      0x08

#define ISO15693_RES_ERR_NOT_SUPP       0x01
#define ISO15693_RES_ERR_NOT_REC        0x02
#define ISO15693_RES_ERR_OPT_NOT_SUPP   0x03
#define ISO15693_RES_ERR_GENERIC        0x0F
#define ISO15693_RES_ERR_BLK_NOT_AVL    0x10
#define ISO15693_RES_ERR_BLK_ALRD_LKD   0x11
#define ISO15693_RES_ERR_BLK_CHG_LKD    0x12
#define ISO15693_RES_ERR_BLK_NOT_PRGR   0x13
#define ISO15693_RES_ERR_BLK_NOT_LKD    0x14

#define ISO15693_RES_INVENTORY_DSFID    0x00

#define ISO15693_MIN_FRAME_SIZE         4

#define ISO15693_CRC16_SIZE             2 /* Bytes */
#define ISO15693_CRC16_POLYNORMAL       0x8408
#define ISO15693_CRC16_PRESET           0xFFFF

typedef uint8_t ISO15693UidType[8];

void ISO15693AppendCRC(uint8_t* FrameBuf, uint16_t FrameBufSize);
bool ISO15693CheckCRC(void* FrameBuf, uint16_t FrameBufSize);

INLINE
bool ISO15693CompareUid(uint8_t* Uid1, uint8_t* Uid2)
{
    if (    (Uid1[0] == Uid2[7])
        &&  (Uid1[1] == Uid2[6])
        &&  (Uid1[2] == Uid2[5])
        &&  (Uid1[3] == Uid2[4])
        &&  (Uid1[4] == Uid2[3])
        &&  (Uid1[5] == Uid2[2])
        &&  (Uid1[6] == Uid2[1])
        &&  (Uid1[7] == Uid2[0]) ) {
        return true;
    } else {
        return false;
    }
}

INLINE
void ISO15693CopyUid(uint8_t* DstUid, uint8_t* SrcUid)
{
    DstUid[0] = SrcUid[7];
    DstUid[1] = SrcUid[6];
    DstUid[2] = SrcUid[5];
    DstUid[3] = SrcUid[4];
    DstUid[4] = SrcUid[3];
    DstUid[5] = SrcUid[2];
    DstUid[6] = SrcUid[1];
    DstUid[7] = SrcUid[0];
}

INLINE
bool ISO15693Addressed(uint8_t* Buffer) {
    return (Buffer[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_ADDRESS); /* if the flag is set, the command is addressed */
}


INLINE
bool ISO15693AddressedLegacy(uint8_t* Buffer, uint8_t* MyUid) {
    if (Buffer[0] & ISO15693_REQ_FLAG_ADDRESS) {
        /* Addressed mode */
        if ( ISO15693CompareUid(&Buffer[2], MyUid) ) {
            /* Our UID addressed */
            return true;
        } else {
            /* Our UID not addressed */
            return false;
        }
    } else {
        /* Non-Addressed mode */
        return true;
    }
}

#endif /* ISO15693_3_H_ */
