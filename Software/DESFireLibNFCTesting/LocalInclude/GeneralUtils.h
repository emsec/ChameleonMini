/* GeneralUtils.h */

#ifndef __GENERAL_LOCAL_UTILS_H__
#define __GENERAL_LOCAL_UTILS_H__

#include "Config.h"

#define MIN(x, y)                    ((x) <= (y) ? (x) : (y))
#define MAX(x, y)                    ((x) <= (y) ? (y) : (y))

#define BITS_PER_BYTE                (8)
#define ASBITS(byteCount)            ((byteCount) * BITS_PER_BYTE)
#define ASBYTES(bitCount)            ((bitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE)

static inline void Int32ToByteBuffer(uint8_t *byteBuffer, int32_t int32Value) {
    if (byteBuffer == NULL) {
        return;
    }
    byteBuffer[0] = (uint8_t)(int32Value & 0x000000ff);
    byteBuffer[1] = (uint8_t)((int32Value >> 8) & 0x000000ff);
    byteBuffer[2] = (uint8_t)((int32Value >> 16) & 0x000000ff);
    byteBuffer[3] = (uint8_t)((int32Value >> 24) & 0x000000ff);
}

void Int24ToByteBuffer(uint8_t *byteBuffer, uint32_t int24Value) {
    if (byteBuffer == NULL) {
        return;
    }
    byteBuffer[0] = (uint8_t)(int24Value & 0x0000ff);
    byteBuffer[1] = (uint8_t)((int24Value >> 8) & 0x0000ff);
    byteBuffer[2] = (uint8_t)((int24Value >> 16) & 0x0000ff);
}

#endif
