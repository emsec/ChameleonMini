#include "Common.h"

const uint8_t PROGMEM BitReverseByteTable[256] = {
#   define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#   define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#   define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
    R6(0), R6(2), R6(1), R6(3)
};

const uint8_t PROGMEM OddParityByteTable[256] = {
    1, 	0, 	0, 	1, 	0, 	1, 	1,	0,	0,	1,	1,	0, 	1, 	0, 	0, 	1,
    0, 	1, 	1,	0, 	1, 	0, 	0, 	1, 	1, 	0, 	0, 	1, 	0, 	1, 	1, 	0,
    0,	1,	1,	0,	1,	0,	0,	1,	1,	0,	0,	1,	0,	1,	1,	0,
    1,	0,	0,	1,	0,	1,	1,	0,	0,	1,	1,	0,	1,	0,	0,	1,
    0,	1,	1,	0,	1,	0,	0,	1,	1,	0,	0,	1,	0,	1,	1,	0,
    1,	0,	0,	1,	0,	1,	1,	0,	0,	1,	1,	0,	1,	0,	0,	1,
    1,	0,	0,	1,	0,	1,	1,	0,	0,	1,	1,	0,	1,	0,	0,	1,
    0,	1,	1,	0,	1,	0,	0,	1,	1,	0,	0,	1,	0,	1,	1,	0,
    0,	1,	1,	0,	1,	0,	0,	1,	1,	0,	0,	1,	0,	1,	1,	0,
    1,	0,	0,	1,	0,	1,	1,	0,	0,	1,	1,	0,	1,	0,	0,	1,
    1,	0,	0,	1,	0,	1,	1,	0,	0,	1,	1,	0,	1,	0,	0,	1,
    0,	1,	1,	0,	1,	0,	0,	1,	1,	0,	0,	1,	0,	1,	1,	0,
    1,	0,	0,	1,	0,	1,	1,	0,	0,	1,	1,	0,	1,	0,	0,	1,
    0,	1,	1,	0,	1,	0,	0,	1,	1,	0,	0,	1,	0,	1,	1,	0,
    0,	1,	1,	0,	1,	0,	0,	1,	1,	0,	0,	1,	0,	1,	1,	0,
    1,	0,	0,	1,	0,	1,	1,	0,	0,	1,	1,	0,	1,	0,	0,	1,
};

uint16_t BufferToHexString(char *HexOut, uint16_t MaxChars, const void *Buffer, uint16_t ByteCount) {
    uint8_t *ByteBuffer = (uint8_t *) Buffer;
    uint16_t CharCount = 0;

    /* Account for '\0' at the end */
    MaxChars--;

    while ((ByteCount > 0) && (MaxChars >= 2)) {
        uint8_t Byte = *ByteBuffer;

        HexOut[0] = NIBBLE_TO_HEXCHAR((Byte >> 4) & 0x0F);
        HexOut[1] = NIBBLE_TO_HEXCHAR((Byte >> 0) & 0x0F);

        HexOut += 2;
        MaxChars -= 2;
        CharCount += 2;
        ByteBuffer++;
        ByteCount -= 1;
    }

    *HexOut = '\0';

    return CharCount;
}

uint16_t HexStringToBuffer(void *Buffer, uint16_t MaxBytes, const char *HexIn) {
    uint8_t *ByteBuffer = (uint8_t *) Buffer;
    uint16_t ByteCount = 0;

    while ((HexIn[0] != '\0') && (HexIn[1] != '\0') && (MaxBytes > 0)) {
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

    if ((HexIn[0] != '\0') && (HexIn[1] == '\0')) {
        /* Odd number of characters */
        return 0;
    }

    return ByteCount;
}
