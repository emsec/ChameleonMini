#ifndef LOG_H_
#define LOG_H_
/** @file */
#include "Common.h"

#define LOG_SIZE	2048
#define FRAM_LOG_ADDR_ADDR	0x4000 // start of the second half of FRAM
#define FRAM_LOG_START_ADDR	0x4002 // directly after the address
#define FRAM_LOG_SIZE		0x3FFE // the whole second half (minus the 2 Bytes of Address)

extern uint8_t LogMem[LOG_SIZE];
extern uint8_t *LogMemPtr;
extern uint16_t LogMemLeft;

/** Enum for log entry type. \note Every entry type has a specific integer value, which can be found in the source code. */
typedef enum {
    /* Generic */
    LOG_INFO_GENERIC			              = 0x10, ///< Unspecific log entry.
    LOG_INFO_CONFIG_SET			              = 0x11, ///< Configuration change.
    LOG_INFO_SETTING_SET		                   = 0x12, ///< Setting change.
    LOG_INFO_UID_SET			              = 0x13, ///< UID change.
    LOG_INFO_RESET_APP			              = 0x20, ///< Application reset.

    /* Codec */
    LOG_INFO_CODEC_RX_DATA		              = 0x40, ///< Currently active codec received data.
    LOG_INFO_CODEC_TX_DATA		              = 0x41, ///< Currently active codec sent data.
    LOG_INFO_CODEC_RX_DATA_W_PARITY		    = 0x42, ///< Currently active codec received data.
    LOG_INFO_CODEC_TX_DATA_W_PARITY		    = 0x43, ///< Currently active codec sent data.
    LOG_INFO_CODEC_SNI_READER_DATA               = 0x44, //< Sniffing codec receive data from reader
    LOG_INFO_CODEC_SNI_READER_DATA_W_PARITY      = 0x45, //< Sniffing codec receive data from reader
    LOG_INFO_CODEC_SNI_CARD_DATA                 = 0x46, //< Sniffing codec receive data from card
    LOG_INFO_CODEC_SNI_CARD_DATA_W_PARITY        = 0x47, //< Sniffing codec receive data from card
    LOG_INFO_CODEC_READER_FIELD_DETECTED         = 0x48, ///< Add logging of the LEDHook case for FIELD_DETECTED

    /* App */
    LOG_INFO_APP_CMD_READ		              = 0x80, ///< Application processed read command.
    LOG_INFO_APP_CMD_WRITE		              = 0x81, ///< Application processed write command.
    LOG_INFO_APP_CMD_INC		                   = 0x84, ///< Application processed increment command.
    LOG_INFO_APP_CMD_DEC		                   = 0x85, ///< Application processed decrement command.
    LOG_INFO_APP_CMD_TRANSFER	                   = 0x86, ///< Application processed transfer command.
    LOG_INFO_APP_CMD_RESTORE	                   = 0x87, ///< Application processed restore command.
    LOG_INFO_APP_CMD_AUTH		              = 0x90, ///< Application processed authentication command.
    LOG_INFO_APP_CMD_HALT		              = 0x91, ///< Application processed halt command.
    LOG_INFO_APP_CMD_UNKNOWN	                   = 0x92, ///< Application processed an unknown command.
    LOG_INFO_APP_AUTHING		                   = 0xA0, ///< Application is in `authing` state.
    LOG_INFO_APP_AUTHED			              = 0xA1, ///< Application is in `auth` state.
    LOG_ERR_APP_AUTH_FAIL		              = 0xC0, ///< Application authentication failed.
    LOG_ERR_APP_CHECKSUM_FAIL	                   = 0xC1, ///< Application had a checksum fail.
    LOG_ERR_APP_NOT_AUTHED		              = 0xC2, ///< Application is not authenticated.

    /* DESFire emulation and development related logging messages (0xEx): */
#ifdef CONFIG_MF_DESFIRE_SUPPORT
#include "Application/DESFire/DESFireLoggingCodesInclude.c"
#endif

    LOG_INFO_SYSTEM_BOOT		                    = 0xFF, ///< Chameleon boots
    LOG_EMPTY 					            = 0x00  ///< Empty Log Entry. This is not followed by a length byte
                                              ///< nor the two systick bytes nor any data.
} LogEntryEnum;

typedef enum {
    LOG_MODE_OFF,
    LOG_MODE_MEMORY,
    LOG_MODE_LIVE
} LogModeEnum;

typedef void (*LogFuncType)(LogEntryEnum Entry, const void *Data, uint8_t Length);

extern LogFuncType CurrentLogFunc;

void LogInit(void);
void LogTick(void);
void LogTask(void);

void LogMemClear(void);
uint16_t LogMemFree(void);
/* XModem callback */
bool LogMemLoadBlock(void *Buffer, uint32_t BlockAddress, uint16_t ByteCount);

void LogSetModeById(LogModeEnum Mode);
bool LogSetModeByName(const char *Mode);
void LogGetModeByName(char *Mode, uint16_t BufferSize);
void LogGetModeList(char *List, uint16_t BufferSize);
void LogSRAMToFRAM(void);

/* Wrapper function to call current logging function */
INLINE void LogEntry(LogEntryEnum Entry, const void *Data, uint8_t Length) { CurrentLogFunc(Entry, Data, Length); }

#endif /* LOG_H_ */
