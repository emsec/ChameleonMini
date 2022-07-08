/*
 * Map.h
 *
 *  Created on: 07.12.2014
 *      Author: sk
 */

#ifndef MAP_H_
#define MAP_H_

#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#ifdef MEMORY_LIMITED_TESTING
#define MAP_TEXT_BUF_SIZE               24
#else
#define MAP_TEXT_BUF_SIZE		32
#endif
#define MAP_MAX_TEXT_SIZE		(MAP_TEXT_BUF_SIZE - 1)

typedef uint8_t MapIdType;
typedef const char *MapTextPtrType;

const typedef struct {
    MapIdType Id;
    const char Text[MAP_TEXT_BUF_SIZE];
} MapEntryType;

bool MapIdToText(const MapEntryType *MapPtr, uint8_t MapSize, MapIdType Id, char *Text, uint16_t MaxBufferSize);
bool MapTextToId(const MapEntryType *MapPtr, uint8_t MapSize, MapTextPtrType Text, MapIdType *IdPtr);
void MapToString(const MapEntryType *MapPtr, uint8_t MapSize, char *String, uint16_t MaxBufferSize);

#endif /* MAP_H_ */
