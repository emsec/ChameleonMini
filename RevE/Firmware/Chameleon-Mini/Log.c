#include "Log.h"
#include "Settings.h"
#include "Terminal/Terminal.h"
#include <avr/pgmspace.h>

static uint8_t LogMem[1024];
static uint8_t* LogMemPtr;
static uint16_t LogMemLeft;
LogFuncType LogFunc;

static const char PROGMEM LogModeTable[][16] = {
    [LOG_MODE_OFF] = "OFF",
    [LOG_MODE_MEMORY] = "MEMORY",
    [LOG_MODE_TERMINAL] = "TERMINAL"
};

static void LogFuncOff(LogEntryEnum Entry, void* Data, uint8_t Length)
{
    /* Do nothing */
}

static void LogFuncMemory(LogEntryEnum Entry, void* Data, uint8_t Length)
{
    if (LogMemLeft >= (Length + 2)) {
        LogMemLeft -= Length + 2;

        uint8_t* DataPtr = (uint8_t*) Data;

        *LogMemPtr++ = (uint8_t) Entry;
        *LogMemPtr++ = (uint8_t) Length;

        while(Length--) {
            *LogMemPtr++ = *DataPtr++;
        }
    } else {
        /* If memory full. Deactivate logmode */
        LogSetModeById(LOG_MODE_OFF);
    }
}

static void LogFuncTerminal(LogEntryEnum Entry, void* Data, uint8_t Length)
{
    TerminalSendByte((uint8_t) Entry);
    TerminalSendByte((uint8_t) Length);
    TerminalSendBlock(Data, Length);
}

void LogInit(void)
{
    LogMemClear();
    LogSetModeById(GlobalSettings.ActiveSettingPtr->LogMode);
}

void LogTick(void)
{

}

void LogTask(void)
{

}

bool LogMemLoadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount)
{
    if (BlockAddress < sizeof(LogMem)) {
        uint8_t* DataPtr = (uint8_t*) Buffer;
        uint8_t* MemPtr = &LogMem[BlockAddress];

        while(ByteCount-- > 0) {
            *DataPtr++ = *MemPtr++;
        }

        return true;
    } else {
        return false;
    }
}

void LogMemClear(void)
{
    uint16_t i;

    for (i=0; i<sizeof(LogMem); i++) {
        LogMem[i] = (uint8_t) LOG_EMPTY;
    }

    LogMemPtr = LogMem;
    LogMemLeft = sizeof(LogMem);
}

uint16_t LogMemFree(void)
{
    return LogMemLeft;
}

void LogSetModeById(LogModeEnum Mode)
{
    GlobalSettings.ActiveSettingPtr->LogMode = Mode;

    switch(Mode) {
    case LOG_MODE_OFF:
        LogFunc = LogFuncOff;
        break;

    case LOG_MODE_MEMORY:
        LogFunc = LogFuncMemory;
        break;

    case LOG_MODE_TERMINAL:
        LogFunc = LogFuncTerminal;
        break;

    default:
        break;
    }

}

bool LogSetModeByName(const char* Name)
{
    uint8_t i;

    for (i=0; i<(sizeof(LogModeTable) / sizeof(*LogModeTable)); i++) {
        if (strcmp_P(Name, LogModeTable[i]) == 0) {
            LogSetModeById(i);
            return true;
        }
    }

    return false;
}

void LogGetModeByName(char* Name, uint16_t BufferSize)
{
    strncpy_P(Name, LogModeTable[GlobalSettings.ActiveSettingPtr->LogMode], BufferSize);
}

void LogGetModeList(char* List, uint16_t BufferSize)
{
    uint8_t i;

    *List = '\0';

    for (i=0; i<(sizeof(LogModeTable) / sizeof(*LogModeTable)); i++) {
        strncat_P(List, LogModeTable[i], BufferSize);

        if (i < (sizeof(LogModeTable) / sizeof(*LogModeTable) - 1)) {
            strncat_P(List, PSTR(","), BufferSize);
        }
    }
}
