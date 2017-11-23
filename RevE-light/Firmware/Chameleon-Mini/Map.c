#include "Map.h"

bool MapIdToText(const MapEntryType* MapPtr, uint8_t MapSize, MapIdType Id, char* Text, uint16_t MaxBufferSize)
{
	while (MapSize--) {
		if (pgm_read_byte(&MapPtr->Id) == Id) {
			strncpy_P(Text, MapPtr->Text, MaxBufferSize);
			return true;
		}

		MapPtr++;
	}

	return false;
}

bool MapTextToId(const MapEntryType* MapPtr, uint8_t MapSize, MapTextPtrType Text, MapIdType* IdPtr)
{
	while (MapSize--) {
		if (strcmp_P(Text, MapPtr->Text) == 0) {
			if (sizeof(MapIdType) == 1) {
				*IdPtr = pgm_read_byte(&MapPtr->Id);
			} else if (sizeof(MapIdType) == 2) {
				*IdPtr = pgm_read_word(&MapPtr->Id);
			}
			return true;
		}

		MapPtr++;
	}

	return false;
}

void MapToString(MapEntryType* MapPtr, uint8_t MapSize, char* String, uint16_t MaxBufferSize)
{
	uint8_t EntriesLeft = MapSize;
	uint16_t BytesLeft = MaxBufferSize;

	while (EntriesLeft > 0) {
		const char* Text = MapPtr->Text;
		char c;

        while( (c = pgm_read_byte(Text)) != '\0') {
        	if (BytesLeft == 0) {
        		return;
        	}

            *String++ = c;
            Text++;
            BytesLeft--;
        }

        if (EntriesLeft > 1) {
        	/* More than one map entries left */
        	if (BytesLeft == 0) {
        		return;
        	}

        	*String++ = ',';
        }

        MapPtr++;
        EntriesLeft--;
	}

	/* Terminate string */
	if (BytesLeft > 0) {
		*String++ = '\0';
		BytesLeft--;
	}
}
