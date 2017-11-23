#include "Common.h"

uint16_t BufferToHexString(char* HexOut, uint16_t MaxChars, const void* Buffer, uint16_t ByteCount)
{
    uint8_t* ByteBuffer = (uint8_t*) Buffer;
    uint16_t CharCount = 0;

    /* Account for '\0' at the end */
    MaxChars--;

    while( (ByteCount > 0) && (MaxChars >= 2) ) {
        uint8_t Byte = *ByteBuffer;

        HexOut[0] = NIBBLE_TO_HEXCHAR( (Byte >> 4) & 0x0F );
        HexOut[1] = NIBBLE_TO_HEXCHAR( (Byte >> 0) & 0x0F );

        HexOut += 2;
        MaxChars -= 2;
        CharCount += 2;
        ByteBuffer++;
        ByteCount -= 1;
    }

    *HexOut = '\0';

    return CharCount;
}

uint16_t HexStringToBuffer(void* Buffer, uint16_t MaxBytes, const char* HexIn)
{
    uint8_t* ByteBuffer = (uint8_t*) Buffer;
    uint16_t ByteCount = 0;

    while( (HexIn[0] != '\0') && (HexIn[1] != '\0') && (MaxBytes > 0) ) {
        if (VALID_HEXCHAR(HexIn[0]) && VALID_HEXCHAR(HexIn[1])) {
            uint8_t Byte = 0;

            Byte |= HEXCHAR_TO_NIBBLE(HexIn[0]) << 4;
            Byte |= HEXCHAR_TO_NIBBLE(HexIn[1]) << 0;

            *ByteBuffer = Byte;

            ByteBuffer++;
            MaxBytes--;
            ByteCount++;
            HexIn += 2;
        } else {
            /* HEX chars only */
            return 0;
        }
    }

    if ( (HexIn[0] != '\0') && (HexIn[1] == '\0') ) {
        /* Odd number of characters */
        return 0;
    }

    return ByteCount;
}





