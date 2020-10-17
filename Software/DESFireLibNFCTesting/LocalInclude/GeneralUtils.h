/* GeneralUtils.h */

#ifndef __GENERAL_LOCAL_UTILS_H__
#define __GENERAL_LOCAL_UTILS_H__

#include "Config.h"

#define MIN(x, y)                    ((x) <= (y) ? (x) : (y))
#define MAX(x, y)                    ((x) <= (y) ? (y) : (y))

static inline void InvalidateAuthState(void) {
    memset(CRYPTO_RNDB_STATE, 0x00, 8);
    AUTHENTICATED = false;
    AUTHENTICATED_PROTO = 0; 
}

static inline void Int32ToByteBuffer(uint8_t *byteBuffer, int32_t int32Value) {
    if(byteBuffer == NULL) {
        return;
    }    
    byteBuffer[0] = (uint8_t) (int32Value & 0x000000ff);
    byteBuffer[1] = (uint8_t) ((int32Value >> 8) & 0x000000ff);
    byteBuffer[2] = (uint8_t) ((int32Value >> 16) & 0x000000ff);
    byteBuffer[3] = (uint8_t) ((int32Value >> 24) & 0x000000ff);
}

void Int24ToByteBuffer(uint8_t *byteBuffer, uint32_t int24Value) {
    if(byteBuffer == NULL) {
        return;
    }    
    byteBuffer[0] = (uint8_t) (int24Value & 0x0000ff);
    byteBuffer[1] = (uint8_t) ((int24Value >> 8) & 0x0000ff);
    byteBuffer[2] = (uint8_t) ((int24Value >> 16) & 0x0000ff);
}

#endif
