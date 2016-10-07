#include "Random.h"

#include <stdlib.h>

void RandomInit(void)
{

}

uint8_t RandomGetByte(void)
{
    return rand() & 0xFF;
}

void RandomGetBuffer(void* Buffer, uint8_t ByteCount)
{
    uint8_t* BufferPtr = (uint8_t*) Buffer;

    while(ByteCount--) {
        *BufferPtr++ = RandomGetByte();
    }
}

void RandomTick(void)
{
    rand();
    rand();
    rand();
    rand();
}
