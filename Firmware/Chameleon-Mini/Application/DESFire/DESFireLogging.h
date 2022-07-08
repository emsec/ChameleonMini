/*
The DESFire stack portion of this firmware source
is free software written by Maxie Dion Schmidt (@maxieds):
You can redistribute it and/or modify
it under the terms of this license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

The complete source distribution of
this firmware is available at the following link:
https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.

Based in part on the original DESFire code created by
@dev-zzo (GitHub handle) [Dmitry Janushkevich] available at
https://github.com/dev-zzo/ChameleonMini/tree/desfire.

This notice must be retained at the top of all source files where indicated.
*/

/*
 * DESFireLogging.h :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_LOGGING_UTILS_H__
#define __DESFIRE_LOGGING_UTILS_H__

#include "../../Log.h"
#include "../../Common.h"
#include "DESFireFirmwareSettings.h"
#include "DESFireMemoryOperations.h"

#ifndef DESFIRE_MIN_INCOMING_LOGSIZE
#define DESFIRE_MIN_INCOMING_LOGSIZE       (1)
#endif
#ifndef DESFIRE_MIN_OUTGOING_LOGSIZE
#define DESFIRE_MIN_OUTGOING_LOGSIZE       (1)
#endif

void DesfireLogEntry(LogEntryEnum LogCode, void *LogDataBuffer, uint16_t BufSize);
void DesfireLogISOStateChange(int state, int logCode);

#ifdef DESFIRE_DEBUGGING
#define DEBUG_PRINT_P(fmtStr, ...)                                    ({ \
    uint8_t logLength = 0;                                               \
    do {                                                                 \
        snprintf_P((char *) __InternalStringBuffer, STRING_BUFFER_SIZE,  \
		         (const char *) fmtStr, ##__VA_ARGS__);                \
	   logLength = StringLength((char *) __InternalStringBuffer,        \
		                       STRING_BUFFER_SIZE);                    \
	   DesfireLogEntry(LOG_INFO_DESFIRE_DEBUGGING_OUTPUT,               \
			         (char *) __InternalStringBuffer, logLength);     \
    } while(0);                                                          \
    })
#else
#define DEBUG_PRINT_P(fmtStr, ...)                               ({})
#endif

#define GetSourceFileLoggingData()                               ({ \
        char *strBuffer;                                            \
        do {                                                        \
	    snprintf_P(__InternalStringBuffer, STRING_BUFFER_SIZE,     \
                   PSTR("@@ LINE #%d in \"%s\" @@"),                \
			    __LINE__, __FILE__);                             \
	    __InternalStringBuffer[STRING_BUFFER_SIZE - 1] = '\0';     \
        } while(0);                                                 \
        strBuffer = __InternalStringBuffer;                         \
        strBuffer;                                                  \
        })

#define GetSymbolNameString(symbolName)                          ({ \
        char *strBuffer;                                            \
        do {                                                        \
        strncpy_P(__InternalStringBuffer, PSTR(#symbolName),        \
                  STRING_BUFFER_SIZE);                              \
        } while(0);                                                 \
        strBuffer = __InternalStringBuffer;                         \
        strBuffer;                                                  \
        })

#define GetHexBytesString(byteArray, arrSize)                ({ \
    char *strBuffer;                                            \
    do {                                                        \
        BufferToHexString(__InternalStringBuffer,               \
                          STRING_BUFFER_SIZE,                   \
                          byteArray, arrSize);                  \
        __InternalStringBuffer[STRING_BUFFER_SIZE - 1] = '\0';  \
    } while(0);                                                 \
    strBuffer = __InternalStringBuffer;                         \
    strBuffer;                                                  \
    })

#endif
