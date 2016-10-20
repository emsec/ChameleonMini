/*
 * ISO14443A.c
 *
 *  Created on: 19.03.2013
 *      Author: skuser
 */

#include "ISO14443-3A.h"

#define CRC_INIT		0x6363
#define CRC_INIT_R		0xC6C6 /* Bit reversed */

#define USE_HW_CRC

#ifdef USE_HW_CRC

uint16_t ISO14443AComputeCRCA(const void* Buffer, uint16_t ByteCount)
{
    uint16_t Checksum;
    const uint8_t* DataPtr = (const uint8_t*) Buffer;

    CRC.CTRL = CRC_RESET0_bm;
    CRC.CHECKSUM1 = (CRC_INIT_R >> 8) & 0xFF;
    CRC.CHECKSUM0 = (CRC_INIT_R >> 0) & 0xFF;
    CRC.CTRL = CRC_SOURCE_IO_gc;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;
        Byte = BitReverseByte(Byte);
        CRC.DATAIN = Byte;
    }

    Checksum = BitReverseByte(CRC.CHECKSUM1) | (BitReverseByte(CRC.CHECKSUM0) << 8);
    CRC.CTRL = CRC_SOURCE_DISABLE_gc;
    return Checksum;
}

#else

#include <util/crc16.h>

uint16_t ISO14443AComputeCRCA(const void* Buffer, uint16_t ByteCount)
{
    uint16_t Checksum = CRC_INIT;
    const uint8_t* DataPtr = (const uint8_t*) Buffer;

    while(ByteCount--) {
        uint8_t Byte = *DataPtr++;
        Checksum = _crc_ccitt_update(Checksum, Byte);
    }

    return Checksum;
}

#endif

void ISO14443AAppendCRCA(void* Buffer, uint16_t ByteCount)
{
    uint16_t Checksum = ISO14443AComputeCRCA(Buffer, ByteCount);
    uint8_t* CrcPtr = (uint8_t*)Buffer + ByteCount;
    CrcPtr[0] = (Checksum >> 0) & 0x00FF;
    CrcPtr[1] = (Checksum >> 8) & 0x00FF;
}

bool ISO14443ACheckCRCA(const void* Buffer, uint16_t ByteCount)
{
    uint16_t Checksum = ISO14443AComputeCRCA(Buffer, ByteCount);
    uint8_t* CrcPtr = (uint8_t*)Buffer + ByteCount;
    return (CrcPtr[0] == ((Checksum >> 0) & 0xFF)) && (CrcPtr[1] == ((Checksum >> 8) & 0xFF));
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

bool ISO14443AWakeUp(void* Buffer, uint16_t* BitCount, uint16_t ATQAValue, bool FromHalt)
{
    uint8_t* DataPtr = (uint8_t*) Buffer;

    if ( ((! FromHalt) && (DataPtr[0] == ISO14443A_CMD_REQA)) ||
         (DataPtr[0] == ISO14443A_CMD_WUPA) ){
        DataPtr[0] = (ATQAValue >> 0) & 0x00FF;
        DataPtr[1] = (ATQAValue >> 8) & 0x00FF;

        *BitCount = ISO14443A_ATQA_FRAME_SIZE;

        return true;
    } else {
        return false;
    }
}

#endif
