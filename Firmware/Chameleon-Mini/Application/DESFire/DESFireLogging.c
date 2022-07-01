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
#include "DESFireISO14443Support.h"
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

#ifdef DESFIRE_DEBUGGING && DESFIRE_DEBUGGING != 0
void DesfireLogEntry(LogEntryEnum LogCode, void *LogDataBuffer, uint16_t BufSize) {
    if (BufSize >= DESFIRE_MIN_OUTGOING_LOGSIZE) {
        LogEntry(LogCode, (void *) LogDataBuffer, BufSize);
    }
}
#else
void DesfireLogEntry(LogEntryEnum LogCode, void *LogDataBuffer, uint16_t BufSize) {}
#endif

#ifdef DESFIRE_DEBUGGING && DESFIRE_DEBUGGING != 0
void DesfireLogISOStateChange(int state, int logCode) {
    const char *statePrintName;
    int logLength = 0;
    do {
        switch (logCode) {
            case ISO14443_4_STATE_EXPECT_RATS:
                statePrintName = PSTR("ISO14443_4_STATE_EXPECT_RATS");
                break;
            case ISO14443_4_STATE_ACTIVE:
                statePrintName = PSTR("ISO14443_4_STATE_ACTIVE");
                break;
            case ISO14443_4_STATE_LAST:
                statePrintName = PSTR("ISO14443_4_STATE_LAST");
                break;
            case ISO14443_3A_STATE_IDLE:
                statePrintName = PSTR("ISO14443_3A_STATE_IDLE");
                break;
            case ISO14443_3A_STATE_READY1:
                statePrintName = PSTR("ISO14443_3A_STATE_READY1");
                break;
            case ISO14443_3A_STATE_READY2:
                statePrintName = PSTR("ISO14443_3A_STATE_READY2");
                break;
            case ISO14443_3A_STATE_ACTIVE:
                statePrintName = PSTR("ISO14443_3A_STATE_ACTIVE");
                break;
            case ISO14443_3A_STATE_HALT:
                statePrintName = PSTR("ISO14443_3A_STATE_HALT");
                break;
            default:
                statePrintName = PSTR("Unknown state");
                break;
        }
    } while (0);
    snprintf_P(STRING_BUFFER_SIZE, (char *) __InternalStringBuffer,
               PSTR("State CHG -- %s"), statePrintName);
    logLength = StringLength((char *) __InternalStringBuffer,
                             STRING_BUFFER_SIZE);
    DesfireLogEntry(LOG_ERR_DESFIRE_GENERIC_ERROR,
                    (char *) __InternalStringBuffer, logLength);
}
#else
void DesfireLogISOStateChange(int state, int logCode) {}
#endif

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
