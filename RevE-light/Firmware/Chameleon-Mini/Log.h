#ifndef LOG_H_
#define LOG_H_

#include "Common.h"

#define LOG_SIZE	2048

typedef enum {
    /* Generic */
    LOG_INFO_GENERIC = 0x10,
    LOG_INFO_CONFIG_SET = 0x11,
    LOG_INFO_SETTING_SET = 0x12,
    LOG_INFO_UID_SET = 0x13,
    LOG_INFO_RESET_APP = 0x20,

    /* Codec */
    LOG_INFO_CODEC_RX_DATA = 0x40,
    LOG_INFO_CODEC_TX_DATA = 0x41,

    /* App */
    LOG_INFO_APP_CMD_READ = 0x80,
    LOG_INFO_APP_CMD_WRITE = 0x81,
    LOG_INFO_APP_CMD_INC = 0x84,
    LOG_INFO_APP_CMD_DEC = 0x85,
    LOG_INFO_APP_CMD_TRANSFER = 0x86,
    LOG_INFO_APP_CMD_RESTORE = 0x87,
    LOG_INFO_APP_CMD_AUTH = 0x90,
    LOG_INFO_APP_CMD_HALT = 0x91,
    LOG_INFO_APP_CMD_UNKNOWN = 0x92,
    LOG_INFO_APP_AUTHING = 0xA0,
    LOG_INFO_APP_AUTHED = 0xA1,
    LOG_ERR_APP_AUTH_FAIL = 0xC0,
    LOG_ERR_APP_CHECKSUM_FAIL = 0xC1,
    LOG_ERR_APP_NOT_AUTHED = 0xC2,

    LOG_EMPTY = 0x00
} LogEntryEnum;

typedef enum {
    LOG_MODE_OFF,
    LOG_MODE_MEMORY,
    LOG_MODE_LIVE
} LogModeEnum;

typedef void (*LogFuncType) (LogEntryEnum Entry, const void* Data, uint8_t Length);

extern LogFuncType CurrentLogFunc;

void LogInit(void);
void LogTick(void);
void LogTask(void);

void LogMemClear(void);
uint16_t LogMemFree(void);
/* XModem callback */
bool LogMemLoadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount);

void LogSetModeById(LogModeEnum Mode);
bool LogSetModeByName(const char* Mode);
void LogGetModeByName(char* Mode, uint16_t BufferSize);
void LogGetModeList(char* List, uint16_t BufferSize);

/* Wrapper function to call current logging function */
INLINE void LogEntry(LogEntryEnum Entry, const void* Data, uint8_t Length) { CurrentLogFunc(Entry, Data, Length); }

#endif /* LOG_H_ */
