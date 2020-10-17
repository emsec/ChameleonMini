#include <stdlib.h>

#include "Random.h"
#include "System.h"

void RandomInit(void) {
    uint32_t randomSeed = (uint32_t) ((uint32_t) (SystemGetSysTick() + 768) | 
                                      ((uint32_t) (SystemGetSysTick() % 512) << 20));
    srand(randomSeed);
}

uint8_t RandomGetByte(void) {
    return rand() & 0xFF;
}

void RandomGetBuffer(void *Buffer, uint8_t ByteCount) {
    uint8_t *BufferPtr = (uint8_t *) Buffer;

    while (ByteCount--) {
        *BufferPtr++ = RandomGetByte();
    }
}

void RandomTick(void) {
    rand();
    rand();
    rand();
    rand();
}
