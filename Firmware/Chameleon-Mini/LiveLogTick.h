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
    uint8_t                  *logBlockDataStart;
    uint8_t                  logBlockDataSize;
    struct LogBlockListNode  *nextBlock;
} LogBlockListNode;

#define LOG_BLOCK_LIST_NODE_SIZE             (sizeof(LogBlockListNode) + 4 - (uint8_t) (sizeof(LogBlockListNode) % 4))
extern LogBlockListNode *LogBlockListBegin;
extern LogBlockListNode *LogBlockListEnd;
extern uint8_t LogBlockListElementCount;

#define LIVE_LOGGER_POST_TICKS               (6)
extern uint8_t LiveLogModePostTickCount;

INLINE bool AtomicAppendLogBlock(LogEntryEnum logCode, uint16_t sysTickTime, const uint8_t *logData, uint8_t logDataSize);
INLINE void FreeLogBlocks(void);
INLINE bool AtomicLiveLogTick(void);
INLINE bool LiveLogTick(void);

INLINE bool
AtomicAppendLogBlock(LogEntryEnum logCode, uint16_t sysTickTime, const uint8_t *logData, uint8_t logDataSize) {
    bool status = true;
    if ((logDataSize + 4 + 3 + LOG_BLOCK_LIST_NODE_SIZE > LogMemLeft) && (LogMemPtr != LogMem)) {
        if (FLUSH_LOGS_ON_SPACE_ERROR) {
            LiveLogTick();
            FreeLogBlocks();
        }
        status = false;
    } else if (logDataSize + 4 + 3 + LOG_BLOCK_LIST_NODE_SIZE <= LogMemLeft) {
        uint8_t alignOffset = 4 - (uint8_t)(((uint16_t) LogMemPtr) % 4);
        uint8_t *logBlockStart = LogMemPtr + alignOffset;
        LogBlockListNode logBlock;
        LogMemPtr += LOG_BLOCK_LIST_NODE_SIZE + alignOffset;
        LogMemLeft -= LOG_BLOCK_LIST_NODE_SIZE + alignOffset;
        logBlock.logBlockDataStart = LogMemPtr;
        logBlock.logBlockDataSize = logDataSize + 4;
        logBlock.nextBlock = 0;
        *(LogMemPtr++) = (uint8_t) logCode;
        *(LogMemPtr++) = logDataSize;
        *(LogMemPtr++) = (uint8_t)(sysTickTime >> 8);
        *(LogMemPtr++) = (uint8_t)(sysTickTime >> 0);
        memcpy(LogMemPtr, logData, logDataSize);
        LogMemPtr += logDataSize;
        LogMemLeft -= logDataSize + 4;
        memcpy(logBlockStart, &logBlock, sizeof(LogBlockListNode));
        if (LogBlockListBegin != NULL && LogBlockListEnd != NULL) {
            LogBlockListEnd->nextBlock = (LogBlockListNode *) logBlockStart;
            LogBlockListEnd = (LogBlockListNode *) logBlockStart;
        } else {
            LogBlockListBegin = LogBlockListEnd = (LogBlockListNode *) logBlockStart;
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
    LogMemLeft = LOG_SIZE;
    LogBlockListBegin = LogBlockListEnd = NULL;
    LogBlockListElementCount = 0;
}

INLINE bool
AtomicLiveLogTick(void) {
    return LiveLogTick();
}

INLINE bool
LiveLogTick(void) {
    LogBlockListNode logBlockCurrent, *tempBlockPtr = NULL;
    memcpy(&logBlockCurrent, LogBlockListBegin, sizeof(LogBlockListNode));
    while (LogBlockListElementCount > 0) {
        TerminalFlushBuffer();
        TerminalSendBlock(logBlockCurrent.logBlockDataStart, logBlockCurrent.logBlockDataSize);
        TerminalFlushBuffer();
        tempBlockPtr = logBlockCurrent.nextBlock;
        if (tempBlockPtr != NULL) {
            memcpy(&logBlockCurrent, tempBlockPtr, sizeof(LogBlockListNode));
        } else {
            break;
        }
        --LogBlockListElementCount;
    }
    FreeLogBlocks();
    LiveLogModePostTickCount = 0;
    return true;
}

#endif
