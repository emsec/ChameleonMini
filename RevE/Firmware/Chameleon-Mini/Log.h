#ifndef LOG_H_
#define LOG_H_

#include "Common.h"

typedef enum {
    /* Generic */
    LOG_INFO_GENERIC = 0x10,
    LOG_INFO_CONFIG_CHANGED = 0x11,

    /* Codec */
    LOG_INFO_RX_DATA = 0x20,
    LOG_INFO_TX_DATA = 0x21,

    /* App */
    LOG_INFO_APP_RESET = 0x30,
    LOG_ERR_APP_AUTH_FAIL = 0x31,
    LOG_ERR_APP_CHECKSUM_FAIL = 0x32,

    LOG_EMPTY = 0xFF
} LogEntryEnum;

typedef enum {
    LOG_MODE_OFF,
    LOG_MODE_MEMORY,
    LOG_MODE_TERMINAL
} LogModeEnum;

typedef void (*LogFuncType) (LogEntryEnum Entry, void* Data, uint8_t Length);

extern LogFuncType LogFunc;

void LogInit(void);
void LogTick(void);
void LogTask(void);

void LogMemClear(void);
uint16_t LogMemFree(void);
/* XModem callback */
bool LogMemLoadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount);

void LogSetModeById(LogModeEnum Mode);
bool LogSetModeByName(const char* Name);
void LogGetModeByName(char* Name, uint16_t BufferSize);
void LogGetModeList(char* List, uint16_t BufferSize);

INLINE void LogEntry(LogEntryEnum Entry, void* Data, uint8_t Length) { LogFunc(Entry, Data, Length); }

#endif /* LOG_H_ */
