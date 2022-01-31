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
 * DESFireLogging.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../../Log.h"
#include "../../Terminal/Terminal.h"

#include "DESFireLogging.h"

#ifdef DESFIRE_DEFAULT_DEBUGGING_MODE
DESFireLoggingMode LocalLoggingMode = DESFIRE_DEFAULT_LOGGING_MODE;
#else
DESFireLoggingMode LocalLoggingMode = DEBUGGING;
#endif

#ifdef DESFIRE_DEFAULT_TESTING_MODE
BYTE LocalTestingMode = DESFIRE_DEFAULT_TESTING_MODE;
#else
BYTE LocalTestingMode = 0x00;
#endif

void DESFireLogErrorMessage(char *fmtMsg, ...) {
    char Format[80];
    char Buffer[80];
    va_list args;
    strcpy_P(Format, fmtMsg);
    va_start(args, fmtMsg);
    vsnprintf(Buffer, sizeof(Buffer), Format, args);
    va_end(args);
    LogEntry(LOG_ERR_DESFIRE_GENERIC_ERROR, Buffer, StringLength(Buffer, 80) + 1);
}

void DESFireLogSourceCodeTODO(char *implNoteMsg, char *srcFileLoggingData) {
    char *bigDataBuffer = (char *) __InternalStringBuffer;
    snprintf_P(bigDataBuffer, STRING_BUFFER_SIZE, PSTR("%s: %s"),
               implNoteMsg, srcFileLoggingData);
    SIZET logMsgBufferSize = StringLength(bigDataBuffer, STRING_BUFFER_SIZE);
    LogEntry(LOG_INFO_DESFIRE_DEBUGGING_OUTPUT, bigDataBuffer, logMsgBufferSize + 1);
}

void DebugPrintP(const char *fmt, ...) {
    char Format[80];
    char Buffer[80];
    va_list args;
    strcpy_P(Format, fmt);
    va_start(args, fmt);
    vsnprintf(Buffer, sizeof(Buffer), Format, args);
    va_end(args);
    TerminalSendString(Buffer);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
