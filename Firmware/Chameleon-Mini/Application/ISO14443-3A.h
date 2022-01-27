/*
 * ISO14443-2A.h
 *
 *  Created on: 19.03.2013
 *      Author: skuser
 */

#ifndef ISO14443_3A_H_
#define ISO14443_3A_H_

#include "../Common.h"
#include <string.h>

#define ISO14443A_UID_SIZE_SINGLE   4    /* bytes */
#define ISO14443A_UID_SIZE_DOUBLE   7
#define ISO14443A_UID_SIZE_TRIPLE   10

#define ISO14443A_CMD_REQA          0x26
#define ISO14443A_CMD_WUPA          0x52
#define ISO14443A_CMD_SELECT_CL1    0x93
#define ISO14443A_CMD_SELECT_CL2    0x95
#define ISO14443A_CMD_SELECT_CL3    0x97
#define ISO14443A_CMD_HLTA          0x50

#define ISO14443A_NVB_AC_START      0x20
#define ISO14443A_NVB_AC_END        0x70

#define IsSelectCmd(Buffer)         ((Buffer[0] == ISO14443A_CMD_SELECT_CL1) || \
				     (Buffer[0] == ISO14443A_CMD_SELECT_CL2) || \
				     (Buffer[0] == ISO14443A_CMD_SELECT_CL3))
#define IsCmdSelectRound1(Buffer)   (IsSelectCmd(Buffer) && (Buffer[1] == ISO14443A_NVB_AC_START))
#define IsCmdSelectRound2(Buffer)   (IsSelectCmd(Buffer) && (Buffer[1] == ISO14443A_NVB_AC_END))

#define ISO14443A_CL_UID_OFFSET     0
#define ISO14443A_CL_UID_SIZE       4
#define ISO14443A_CL_BCC_OFFSET     4
#define ISO14443A_CL_BCC_SIZE       1 /* Byte */
#define ISO14443A_CL_FRAME_SIZE     ((ISO14443A_CL_UID_SIZE + ISO14443A_CL_BCC_SIZE) * 8)    /* UID[N...N+3] || BCCN */
#define ISO14443A_SAK_INCOMPLETE                0x24        // 0x04
#define ISO14443A_SAK_COMPLETE_COMPLIANT        0x20
#define ISO14443A_SAK_COMPLETE_NOT_COMPLIANT    0x00

#define ISO14443A_ATQA_FRAME_SIZE_BYTES   (2)                 /* in Bytes */
#define ISO14443A_ATQA_FRAME_SIZE         (2 * BITS_PER_BYTE) /* in Bits */
#define ISO14443A_SAK_FRAME_SIZE          (3 * BITS_PER_BYTE) /* in Bits */
#define ISO14443A_HLTA_FRAME_SIZE         (2 * BITS_PER_BYTE) /* in Bits */

#define ISO14443A_UID0_RANDOM       0x08
#define ISO14443A_UID0_CT           0x88

#define ISO14443A_CRCA_SIZE         2

#define ISO14443A_CALC_BCC(ByteBuffer) \
    ( ByteBuffer[0] ^ ByteBuffer[1] ^ ByteBuffer[2] ^ ByteBuffer[3] )

void ISO14443AAppendCRCA(void *Buffer, uint16_t ByteCount);
bool ISO14443ACheckCRCA(const void *Buffer, uint16_t ByteCount);

INLINE bool ISO14443ASelect(void *Buffer, uint16_t *BitCount, uint8_t *UidCL, uint8_t SAKValue);
INLINE bool ISO14443AWakeUp(void *Buffer, uint16_t *BitCount, uint16_t ATQAValue, bool FromHalt);

INLINE
bool ISO14443ASelect(void *Buffer, uint16_t *BitCount, uint8_t *UidCL, uint8_t SAKValue) {
    uint8_t *DataPtr = (uint8_t *) Buffer;
    uint8_t NVB = DataPtr[1];

    switch (NVB) {
        case ISO14443A_NVB_AC_START:
            /* Start of anticollision procedure.
             * Send whole UID CLn + BCC */
            DataPtr[0] = UidCL[0];
            DataPtr[1] = UidCL[1];
            DataPtr[2] = UidCL[2];
            DataPtr[3] = UidCL[3];
            DataPtr[4] = ISO14443A_CALC_BCC(DataPtr);
            *BitCount = ISO14443A_CL_FRAME_SIZE;
            return false;

        case ISO14443A_NVB_AC_END:
            /* End of anticollision procedure.
             * Send SAK CLn if we are selected. */
            if ((DataPtr[2] == UidCL[0]) &&
                    (DataPtr[3] == UidCL[1]) &&
                    (DataPtr[4] == UidCL[2]) &&
                    (DataPtr[5] == UidCL[3])) {
                DataPtr[0] = SAKValue;
                ISO14443AAppendCRCA(Buffer, 1);
                *BitCount = ISO14443A_SAK_FRAME_SIZE;
                return true;
            } else {
                /* We have not been selected. Don't send anything. */
                *BitCount = 0;
                return false;
            }
        default: {
            uint8_t CollisionByteCount = ((NVB >> 4) & 0x0f) - 2;
            uint8_t CollisionBitCount  = (NVB >> 0) & 0x0f;
            uint8_t mask = 0xFF >> (8 - CollisionBitCount);
            // Since the UidCL does not contain the BCC, we have to distinguish here
            if (
                ((CollisionByteCount == 5 || (CollisionByteCount == 4 && CollisionBitCount > 0)) && memcmp(UidCL, &DataPtr[2], 4) == 0 && (ISO14443A_CALC_BCC(UidCL) & mask) == (DataPtr[6] & mask))
                ||
                (CollisionByteCount == 4 && CollisionBitCount == 0 && memcmp(UidCL, &DataPtr[2], 4) == 0)
                ||
                (CollisionByteCount < 4 && memcmp(UidCL, &DataPtr[2], CollisionByteCount) == 0 && (UidCL[CollisionByteCount] & mask) == (DataPtr[CollisionByteCount + 2] & mask))
            ) {
                DataPtr[0] = UidCL[0];
                DataPtr[1] = UidCL[1];
                DataPtr[2] = UidCL[2];
                DataPtr[3] = UidCL[3];
                DataPtr[4] = ISO14443A_CALC_BCC(DataPtr);

                *BitCount = ISO14443A_CL_FRAME_SIZE;
            } else {
                *BitCount = 0;
            }
            return false;
        }
            /* TODO: No anticollision supported */
        *BitCount = 0;
        return false;
    }
}

#ifdef CONFIG_MF_DESFIRE_SUPPORT
bool ISO14443ASelectDesfire(void *Buffer, uint16_t *BitCount, uint8_t *UidCL, uint8_t SAKValue);
#endif

INLINE
bool ISO14443AWakeUp(void *Buffer, uint16_t *BitCount, uint16_t ATQAValue, bool FromHalt) {
    uint8_t *DataPtr = (uint8_t *) Buffer;

    if (((! FromHalt) && (DataPtr[0] == ISO14443A_CMD_REQA)) ||
            (DataPtr[0] == ISO14443A_CMD_WUPA)) {
        DataPtr[0] = (ATQAValue >> 0) & 0x00FF;
        DataPtr[1] = (ATQAValue >> 8) & 0x00FF;

        *BitCount = ISO14443A_ATQA_FRAME_SIZE;

        return true;
    } else {
        *BitCount = 0;

        return false;
    }
}

#endif
