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

#define ODD_PARITY(Value) (parity_even_bit(Value) ? 0 : 1)

#define INLINE \
    static inline __attribute__((always_inline))

#define NIBBLE_TO_HEXCHAR(x) ( (x) < 0x0A ? (x) + '0' : (x) + 'A' - 0x0A )
#define HEXCHAR_TO_NIBBLE(x) ( (x) < 'A'  ? (x) - '0' : (x) - 'A' + 0x0A )
#define VALID_HEXCHAR(x) (  ( (x) >= '0' && (x) <= '9' ) || ( (x) >= 'A' && (x) <= 'F' ) )
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define MAX(x,y) ( (x) > (y) ? (x) : (y) )

#define BITS_PER_BYTE 8

uint16_t BufferToHexString(char* HexOut, uint16_t MaxChars, const void* Buffer, uint16_t ByteCount);
uint16_t HexStringToBuffer(void* Buffer, uint16_t MaxBytes, const char* HexIn);

INLINE uint8_t StringLength(const char* Str, uint8_t MaxLen)
{
	uint8_t StrLen = 0;

	while(MaxLen > 0) {
		if (*Str++ == '\0')
			break;

		MaxLen--;
		StrLen++;
	}

	return StrLen;
}

#endif /* COMMON_H_ */
