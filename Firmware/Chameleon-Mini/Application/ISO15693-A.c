#include "ISO15693-A.h"
#include "../Common.h"
#include <util/crc16.h>

//Refer to ISO/IEC 15693-3:2001 page 41
uint16_t calculateCRC(void* FrameBuf, uint16_t FrameBufSize) 
{
    uint16_t reg = ISO15693_CRC16_PRESET;
    uint8_t i, j;

    uint8_t *DataPtr = (uint8_t *)FrameBuf;

    for(i = 0; i < FrameBufSize; i++) {
        reg = reg ^ *DataPtr++;
        for (j = 0; j < 8; j++) {
            if (reg & 0x0001) {
                reg = (reg >> 1) ^ ISO15693_CRC16_POLYNORMAL;
            } else {
                reg = (reg >> 1);
            }
        }
    }

    return ~reg;  
}

void ISO15693AppendCRC(uint8_t* FrameBuf, uint16_t FrameBufSize)
{
    uint16_t crc;
    
    crc = calculateCRC(FrameBuf, FrameBufSize);

    uint8_t crcLb = crc & 0xFF;
    uint8_t crcHb = crc >> 8;


    FrameBuf[FrameBufSize] = crcLb;
    FrameBuf[FrameBufSize + 1] = crcHb;    
}

bool ISO15693CheckCRC(void* FrameBuf, uint16_t FrameBufSize)
{
    uint16_t crc;
    uint8_t *DataPtr = (uint8_t *)FrameBuf;
    
    crc = calculateCRC(DataPtr, FrameBufSize);
    
    uint8_t crcLb = crc & 0xFF;
    uint8_t crcHb = crc >> 8;
    
    return (DataPtr[FrameBufSize] == crcLb && DataPtr[FrameBufSize + 1] == crcHb);
}

/*
 * ISO15693PrepareFrame
 * 
 * This function validates frame lenght and sets pointers in 'frame' struct
 * to relevant byte(s) of 'FrameBuf'. Also sets frame.addressed as true if
 * the command is addressed
 * 
 * Returns:
 *  - true:  Frame is valid and a response is needed
 *  - false: Request is not addressed to us
 *           Frame is not valid because it's too short or CRC is wrong
 */
bool ISO15693PrepareFrame(uint8_t* FrameBuf, uint16_t FrameBytes, CurrentFrame* FrameStruct, uint8_t* MyUid)
{
    if ((FrameBytes < ISO15693_MIN_FRAME_SIZE) || !ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE))
        /* malformed frame */
        return false;

    /* following declarations are not dependent on addressed/unaddressed state */
    FrameStruct -> Flags        = &FrameBuf[ISO15693_ADDR_FLAGS];
    FrameStruct -> Command      = &FrameBuf[ISO15693_REQ_ADDR_CMD];

    if(!(FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_INVENTORY)) /* if inventory flag is not set */
        FrameStruct -> Addressed = (FrameBuf[ISO15693_ADDR_FLAGS] & ISO15693_REQ_FLAG_ADDRESS); /* check for addressed flag */
    else /* otherwise always false */
        FrameStruct -> Addressed = false;

    if (FrameStruct -> Addressed)
        /* UID sits between CMD and PARAM */
        FrameStruct -> Parameters = &FrameBuf[ISO15693_REQ_ADDR_PARAM + ISO15693_GENERIC_UID_SIZE];
    else
        FrameStruct -> Parameters = &FrameBuf[ISO15693_REQ_ADDR_PARAM];

    FrameStruct -> ParamLen = FrameBuf + (FrameBytes - ISO15693_CRC16_SIZE) - (FrameStruct -> Parameters);

    if (FrameStruct -> Addressed && !ISO15693CompareUid(&FrameBuf[ISO15693_REQ_ADDR_PARAM], MyUid)) {
        /* addressed request but we're not the addressee */
        return false;
    } else {
        return true;
    }
}
