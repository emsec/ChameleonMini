/*
 * Common.h
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdbool.h>
#include <util/parity.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/io.h>

#define ODD_PARITY(Value) OddParityBit(Value)

/* This function type has to be used for all the interrupt handlers that have to be changed at runtime: */
#define ISR_SHARED \
    void __attribute__((signal))

#define INLINE \
    static inline __attribute__((always_inline))

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

#define ARRAY_COUNT(x) \
    (sizeof(x) / sizeof(x[0]))

#define NIBBLE_TO_HEXCHAR(x) ( (x) < 0x0A ? (x) + '0' : (x) + 'A' - 0x0A )
#define HEXCHAR_TO_NIBBLE(x) ( (x) < 'A'  ? (x) - '0' : (x) - 'A' + 0x0A )
#define VALID_HEXCHAR(x) (  ( (x) >= '0' && (x) <= '9' ) || ( (x) >= 'A' && (x) <= 'F' ) )
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define MAX(x,y) ( (x) > (y) ? (x) : (y) )
#define SYSTICK_DIFF(since) ((uint16_t) (SystemGetSysTick() - since))
#define SYSTICK_DIFF_100MS(since) (SYSTICK_DIFF(since) / 100)

#define BITS_PER_BYTE 8
#define ARCH_BIG_ENDIAN

uint16_t BufferToHexString(char *HexOut, uint16_t MaxChars, const void *Buffer, uint16_t ByteCount);
uint16_t HexStringToBuffer(void *Buffer, uint16_t MaxBytes, const char *HexIn);

INLINE uint8_t BitReverseByte(uint8_t Byte) {
    extern const uint8_t PROGMEM BitReverseByteTable[];

    return pgm_read_byte(&BitReverseByteTable[Byte]);
}

INLINE uint8_t OddParityBit(uint8_t Byte) {
    extern const uint8_t PROGMEM OddParityByteTable[];

    return pgm_read_byte(&OddParityByteTable[Byte]);
}

INLINE uint8_t StringLength(const char *Str, uint8_t MaxLen) {
    uint8_t StrLen = 0;

    while (MaxLen > 0) {
        if (*Str++ == '\0')
            break;

        MaxLen--;
        StrLen++;
    }

    return StrLen;
}

#endif /* COMMON_H_ */
