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
