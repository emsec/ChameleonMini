/*
 * ISO14443A.c
 *
 *  Created on: 19.03.2013
 *      Author: skuser
 */

#include "ISO14443-3A.h"

void ISO14443AAppendCRCA(void* Buffer, uint16_t ByteCount) {
    uint16_t Checksum = 0x6363;
    uint8_t* DataPtr = (uint8_t*) Buffer;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;

        Byte ^= (uint8_t) (Checksum & 0x00FF);
        Byte ^= Byte << 4;

        Checksum = (Checksum >> 8) ^ ( (uint16_t) Byte << 8 ) ^
                ( (uint16_t) Byte << 3 ) ^ ( (uint16_t) Byte >> 4 );
    }

    *DataPtr++ = (Checksum >> 0) & 0x00FF;
    *DataPtr = (Checksum >> 8) & 0x00FF;
}

bool ISO14443ACheckCRCA(void* Buffer, uint16_t ByteCount)
{
    uint16_t Checksum = 0x6363;
    uint8_t* DataPtr = (uint8_t*) Buffer;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;

        Byte ^= (uint8_t) (Checksum & 0x00FF);
        Byte ^= Byte << 4;

        Checksum = (Checksum >> 8) ^ ( (uint16_t) Byte << 8 ) ^
                ( (uint16_t) Byte << 3 ) ^ ( (uint16_t) Byte >> 4 );
    }

    return (DataPtr[0] == ((Checksum >> 0) & 0xFF)) && (DataPtr[1] == ((Checksum >> 8) & 0xFF));
}

#if 0
bool ISO14443ASelect(void* Buffer, uint16_t* BitCount, uint8_t* UidCL, uint8_t SAKValue)
{
    uint8_t* DataPtr = (uint8_t*) Buffer;
    uint8_t NVB = DataPtr[1];
    //uint8_t CollisionByteCount = (NVB >> 4) & 0x0F;
    //uint8_t CollisionBitCount =  (NVB >> 0) & 0x0F;

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
        if (    (DataPtr[2] == UidCL[0]) &&
                (DataPtr[3] == UidCL[1]) &&
                (DataPtr[4] == UidCL[2]) &&
                (DataPtr[5] == UidCL[3]) ) {

            DataPtr[0] = SAKValue;
            ISO14443AAppendCRCA(Buffer, 1);

            *BitCount = ISO14443A_SAK_FRAME_SIZE;
            return true;
        } else {
            /* We have not been selected. Don't send anything. */
            *BitCount = 0;
            return false;
        }
    default:
        /* TODO: No anticollision supported */
        *BitCount = 0;
        return false;
    }
}

bool ISO14443AWakeUp(void* Buffer, uint16_t* BitCount, uint16_t ATQAValue)
{
    uint8_t* DataPtr = (uint8_t*) Buffer;

    if ( (DataPtr[0] == ISO14443A_CMD_REQA) || (DataPtr[0] == ISO14443A_CMD_WUPA) ){
        DataPtr[0] = (ATQAValue >> 0) & 0x00FF;
        DataPtr[1] = (ATQAValue >> 8) & 0x00FF;

        *BitCount = ISO14443A_ATQA_FRAME_SIZE;

        return true;
    } else {
        return false;
    }
}
#endif
