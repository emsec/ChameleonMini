/* LiveLogTick.h : Handle flushing of live logging buffers out through USB
 *                 by an atomic code block with interrupts disabled.
 *                 If there are many logs being generated at once, this will maintain
 *                 consistency in the returned buffers and prevent the contents of
 *                 USB serial data from getting jumbled or concatenated.
 */

#ifndef __LIVE_LOG_TICK_H__
#define __LIVE_LOG_TICK_H__

#include <inttypes.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "LUFADescriptors.h"

#include "Log.h"
#include "Terminal/Terminal.h"

#define cli_memory() __asm volatile( "cli" ::: "memory" )
#define sei_memory() __asm volatile( "sei" ::: "memory" )

#ifndef FLUSH_LOGS_ON_SPACE_ERROR
#define FLUSH_LOGS_ON_SPACE_ERROR       (1)
#endif

typedef struct LogBlockListNode {
    uint8_t                  *logBlockStart;
    uint8_t                  logBlockSize;
    struct LogBlockListNode  *nextBlock;
} LogBlockListNode;

extern LogBlockListNode *LogBlockListBegin;
extern LogBlockListNode *LogBlockListEnd;
extern uint8_t LogBlockListElementCount;

#define LIVE_LOGGER_POST_TICKS               (3)
extern uint8_t LiveLogModePostTickCount;

INLINE bool AtomicAppendLogBlock(LogEntryEnum logCode, uint16_t sysTickTime, const uint8_t *logData, uint8_t logDataSize);
INLINE void FreeLogBlocks(void);
INLINE bool AtomicLiveLogTick(void);
INLINE bool LiveLogTick(void);

INLINE bool
AtomicAppendLogBlock(LogEntryEnum logCode, uint16_t sysTickTime, const uint8_t *logData, uint8_t logDataSize) {
    bool status = true;
    if ((logDataSize + 4 > LogMemLeft) && (LogMemPtr != LogMem)) {
        if (FLUSH_LOGS_ON_SPACE_ERROR) {
            LiveLogTick();
            FreeLogBlocks();
        }
        status = false;
    } else if (logDataSize + 4 <= LogMemLeft) {
        LogBlockListNode *logBlock = (LogBlockListNode *) malloc(sizeof(LogBlockListNode));
        logBlock->logBlockStart = LogMemPtr;
        logBlock->logBlockSize = logDataSize + 4;
        logBlock->nextBlock = NULL;
        *(LogMemPtr++) = logCode;
        *(LogMemPtr++) = logDataSize;
        *(LogMemPtr++) = (uint8_t)(sysTickTime >> 8);
        *(LogMemPtr++) = (uint8_t)(sysTickTime >> 0);
        memcpy(LogMemPtr, logData, logDataSize);
        LogMemPtr += logDataSize;
        LogMemLeft -= logDataSize + 4;
        if (LogBlockListBegin != NULL && LogBlockListEnd != NULL) {
            LogBlockListEnd->nextBlock = logBlock;
            LogBlockListEnd = logBlock;
        } else {
            LogBlockListBegin = LogBlockListEnd = logBlock;
        }
        ++LogBlockListElementCount;
    } else {
        status = false;
    }
    return status;
}

INLINE void
FreeLogBlocks(void) {
    LogMemPtr = &LogMem[0];
    LogBlockListNode *logBlockCurrent = LogBlockListBegin;
    LogBlockListNode *logBlockNext = NULL;
    while (logBlockCurrent != NULL) {
        logBlockNext = logBlockCurrent->nextBlock;
        LogMemLeft += logBlockCurrent->logBlockSize;
        free(logBlockCurrent);
        logBlockCurrent = logBlockNext;
    }
    LogBlockListBegin = LogBlockListEnd = NULL;
    LogBlockListElementCount = 0;
}

INLINE bool
AtomicLiveLogTick(void) {
    bool status;
    status = LiveLogTick();
    return status;
}

INLINE bool
LiveLogTick(void) {
    bool status = LogBlockListBegin == NULL;
    LogBlockListNode *logBlockCurrent = LogBlockListBegin;
    while (logBlockCurrent != NULL && LogBlockListElementCount > 0) {
        TerminalFlushBuffer();
        TerminalSendBlock(logBlockCurrent->logBlockStart, logBlockCurrent->logBlockSize);
        TerminalFlushBuffer();
        logBlockCurrent = logBlockCurrent->nextBlock;
    }
    FreeLogBlocks();
    LiveLogModePostTickCount = 0x00;
    return status;
}

#endif
