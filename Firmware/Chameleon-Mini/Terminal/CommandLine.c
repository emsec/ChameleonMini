/*
 * CommandLine.c
 *
 *  Created on: 04.05.2013
 *      Author: skuser
 */

#include "CommandLine.h"
#include "Settings.h"
#include "System.h"

#define CHAR_GET_MODE   		'?'     /* <Command>? */
#define CHAR_SET_MODE   		'='     /* <Command>=<Param> */
#define CHAR_EXEC_MODE  		'\0'    /* <Command> */
#define CHAR_EXEC_MODE_PARAM 	' '	   /* <Command> <Param> ... <ParamN> */

#define IS_COMMAND_DELIMITER(c) ( \
  ((c) == CHAR_EXEC_MODE) || ((c) == CHAR_GET_MODE) || ((c) == CHAR_SET_MODE) || ((c) == CHAR_EXEC_MODE_PARAM) \
)

#define IS_CHARACTER(c) ( \
  ( ((c) >= 'A') && ((c) <= 'Z') ) || \
  ( ((c) >= 'a') && ((c) <= 'z') ) || \
  ( ((c) >= '0') && ((c) <= '9') ) || \
  ( ((c) == '_') ) || \
  ( ((c) == CHAR_GET_MODE) || ((c) == CHAR_SET_MODE) || ((c) == CHAR_EXEC_MODE_PARAM) ) \
)

#define IS_LOWERCASE(c) ( ((c) >= 'a') && ((c) <= 'z') )
#define TO_UPPERCASE(c) ( (c) - 'a' + 'A' )

#define IS_WHITESPACE(c) ( ((c) == ' ') || ((c) == '\t') )

#define NO_FUNCTION    ((void*) 0)

#define STATUS_MESSAGE_TRAILER    "\r\n"
#define OPTIONAL_ANSWER_TRAILER   "\r\n"

/* Include all command functions */
#include "Commands.h"

const PROGMEM CommandEntryType CommandTable[] = {
    {
        .Command    = COMMAND_VERSION,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = CommandGetVersion,
    },
    {
        .Command    = COMMAND_CONFIG,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetConfig,
        .GetFunc    = CommandGetConfig
    },
    {
        .Command    = COMMAND_UID,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetUid,
        .GetFunc    = CommandGetUid
    },
    {
        .Command    = COMMAND_READONLY,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .GetFunc    = CommandGetReadOnly,
        .SetFunc    = CommandSetReadOnly

    },
    {
        .Command    = COMMAND_UPLOAD,
        .ExecFunc   = CommandExecUpload,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
    {
        .Command    = COMMAND_DOWNLOAD,
        .ExecFunc   = CommandExecDownload,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
    {
        .Command    = COMMAND_RESET,
        .ExecFunc   = CommandExecReset,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
#ifdef SUPPORT_FIRMWARE_UPGRADE
    {
        .Command    = COMMAND_UPGRADE,
        .ExecFunc   = CommandExecUpgrade,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
#endif
    {
        .Command    = COMMAND_MEMSIZE,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = CommandGetMemSize
    },
    {
        .Command    = COMMAND_UIDSIZE,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = CommandGetUidSize
    },
    {
        .Command    = COMMAND_RBUTTON,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetRButton,
        .GetFunc    = CommandGetRButton
    },
    {
        .Command    = COMMAND_RBUTTON_LONG,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetRButtonLong,
        .GetFunc    = CommandGetRButtonLong
    },
    {
        .Command    = COMMAND_LBUTTON,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetLButton,
        .GetFunc    = CommandGetLButton
    },
    {
        .Command    = COMMAND_LBUTTON_LONG,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetLButtonLong,
        .GetFunc    = CommandGetLButtonLong
    },
    {
        .Command    = COMMAND_LEDGREEN,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetLedGreen,
        .GetFunc    = CommandGetLedGreen
    },
    {
        .Command    = COMMAND_LEDRED,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetLedRed,
        .GetFunc    = CommandGetLedRed
    },
    {
        .Command    = COMMAND_PIN,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetPin,
        .GetFunc    = CommandGetPin
    },
    {
        .Command    = COMMAND_LOGMODE,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetLogMode,
        .GetFunc    = CommandGetLogMode
    },
    {
        .Command    = COMMAND_LOGMEM,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = CommandGetLogMem
    },
    {
        .Command    = COMMAND_LOGDOWNLOAD,
        .ExecFunc   = CommandExecLogDownload,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
    {
        .Command    = COMMAND_STORELOG,
        .ExecFunc   = CommandExecStoreLog,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
    {
        .Command    = COMMAND_LOGCLEAR,
        .ExecFunc   = CommandExecLogClear,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
    {
        .Command	= COMMAND_SETTING,
        .ExecFunc	= NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc	= CommandSetSetting,
        .GetFunc	= CommandGetSetting
    },
    {
        .Command	= COMMAND_CLEAR,
        .ExecFunc	= CommandExecClear,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc	= NO_FUNCTION,
        .GetFunc	= NO_FUNCTION
    },
    {
        .Command	= COMMAND_STORE,
        .ExecFunc	= CommandExecStore,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc	= NO_FUNCTION,
        .GetFunc	= NO_FUNCTION
    },
    {
        .Command	= COMMAND_RECALL,
        .ExecFunc	= CommandExecRecall,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc	= NO_FUNCTION,
        .GetFunc	= NO_FUNCTION
    },
    {
        .Command    = COMMAND_CHARGING,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = CommandGetCharging
    },
    {
        .Command    = COMMAND_HELP,
        .ExecFunc   = CommandExecHelp,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
    {
        .Command	= COMMAND_RSSI,
        .ExecFunc 	= NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= CommandGetRssi
    },
    {
        .Command	= COMMAND_SYSTICK,
        .ExecFunc 	= NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= CommandGetSysTick
    },
#ifdef CONFIG_ISO14443A_READER_SUPPORT
    {
        .Command	= COMMAND_SEND_RAW,
        .ExecFunc 	= NO_FUNCTION,
        .ExecParamFunc = CommandExecParamSendRaw,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= NO_FUNCTION
    },
    {
        .Command	= COMMAND_SEND,
        .ExecFunc 	= NO_FUNCTION,
        .ExecParamFunc = CommandExecParamSend,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= NO_FUNCTION
    },
    {
        .Command	= COMMAND_GETUID,
        .ExecFunc 	= CommandExecGetUid,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= NO_FUNCTION
    },
    {
        .Command	= COMMAND_DUMP_MFU,
        .ExecFunc 	= CommandExecDumpMFU,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= NO_FUNCTION
    },
    {
        .Command	= COMMAND_CLONE_MFU,
        .ExecFunc 	= CommandExecCloneMFU,
        .ExecParamFunc  = NO_FUNCTION,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= NO_FUNCTION
    },
    {
        .Command	= COMMAND_IDENTIFY_CARD,
        .ExecFunc 	= CommandExecIdentifyCard,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc 	= NO_FUNCTION,
        .GetFunc 	= NO_FUNCTION
    },
#endif
    {
        .Command	= COMMAND_TIMEOUT,
        .ExecFunc 	= NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc 	= CommandSetTimeout,
        .GetFunc 	= CommandGetTimeout
    },
    {
        .Command	= COMMAND_THRESHOLD,
        .ExecFunc 	= NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc 	= CommandSetThreshold,
        .GetFunc 	= CommandGetThreshold
    },
    {
        .Command    = COMMAND_AUTOCALIBRATE,
        .ExecFunc   = CommandExecAutocalibrate,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    },
    {
        .Command    = COMMAND_FIELD,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = CommandSetField,
        .GetFunc    = CommandGetField
    },
#ifdef CONFIG_ISO14443A_READER_SUPPORT
    {
        .Command        = COMMAND_CLONE,
        .ExecFunc       = CommandExecClone,
        .ExecParamFunc  = NO_FUNCTION,
        .SetFunc        = NO_FUNCTION,
        .GetFunc        = NO_FUNCTION
    },
#endif
#ifdef CONFIG_ISO15693_SNIFF_SUPPORT
    {
        .Command        = COMMAND_AUTOTHRESHOLD,
        .ExecFunc       = NO_FUNCTION,
        .ExecParamFunc  = NO_FUNCTION,
        .SetFunc        = CommandSetAutoThreshold,
        .GetFunc        = CommandGetAutoThreshold
    },
#endif
#ifdef ENABLE_RUNTESTS_TERMINAL_COMMAND
#include "../Tests/ChameleonTerminalInclude.c"
#endif
#if defined(CONFIG_MF_DESFIRE_SUPPORT) && !defined(DISABLE_DESFIRE_TERMINAL_COMMANDS)
#include "../Application/DESFire/DESFireChameleonTerminalInclude.c"
#endif
    {
        /* This has to be last element */
        .Command    = COMMAND_LIST_END,
        .ExecFunc   = NO_FUNCTION,
        .ExecParamFunc = NO_FUNCTION,
        .SetFunc    = NO_FUNCTION,
        .GetFunc    = NO_FUNCTION
    }
};

#define STATUS_TABLE_ENTRY(Id, Text) \
  { Id, STRINGIFY(Id) ":" Text }

typedef struct {
    CommandStatusIdType Id;
    CommandStatusMessageType Message;
} CommandStatusType;

static const CommandStatusType PROGMEM StatusTable[] = {
    STATUS_TABLE_ENTRY(COMMAND_INFO_OK_ID, COMMAND_INFO_OK),
    STATUS_TABLE_ENTRY(COMMAND_INFO_OK_WITH_TEXT_ID, COMMAND_INFO_OK_WITH_TEXT),
    STATUS_TABLE_ENTRY(COMMAND_INFO_XMODEM_WAIT_ID, COMMAND_INFO_XMODEM_WAIT),
    STATUS_TABLE_ENTRY(COMMAND_ERR_UNKNOWN_CMD_ID, COMMAND_ERR_UNKNOWN_CMD),
    STATUS_TABLE_ENTRY(COMMAND_ERR_INVALID_USAGE_ID, COMMAND_ERR_INVALID_USAGE),
    STATUS_TABLE_ENTRY(COMMAND_ERR_INVALID_PARAM_ID, COMMAND_ERR_INVALID_PARAM),
    STATUS_TABLE_ENTRY(COMMAND_INFO_FALSE_ID, COMMAND_INFO_FALSE),
    STATUS_TABLE_ENTRY(COMMAND_INFO_TRUE_ID, COMMAND_INFO_TRUE),
    STATUS_TABLE_ENTRY(COMMAND_ERR_TIMEOUT_ID, COMMAND_ERR_TIMEOUT),
};

uint16_t TerminalBufferIdx = 0;

void (*CommandLinePendingTaskTimeout)(void) = NO_FUNCTION;  // gets called on Timeout
static bool TaskPending = false;
static uint16_t TaskPendingSince;

static const char *GetStatusMessageP(CommandStatusIdType StatusId) {
    uint8_t i;

    for (i = 0; i < ARRAY_COUNT(StatusTable); i++) {
        if (pgm_read_byte(&StatusTable[i].Id) == StatusId)
            return StatusTable[i].Message;
    }

    return (void *) 0;
}

static CommandStatusIdType CallCommandFunc(
    const CommandEntryType *CommandEntry, char CommandDelimiter, char *pParam) {
    char *pTerminalBuffer = (char *) TerminalBuffer;
    CommandStatusIdType Status = COMMAND_ERR_INVALID_USAGE_ID;

    /* Call appropriate function depending on CommandDelimiter */
    if (CommandDelimiter == CHAR_GET_MODE) {
        CommandGetFuncType GetFunc = pgm_read_ptr(&CommandEntry->GetFunc);
        if (GetFunc != NO_FUNCTION) {
            Status = GetFunc(pTerminalBuffer);
        }
    } else if (CommandDelimiter == CHAR_SET_MODE) {
        CommandSetFuncType SetFunc = pgm_read_ptr(&CommandEntry->SetFunc);
        if (SetFunc != NO_FUNCTION) {
            Status = SetFunc(pTerminalBuffer, pParam);
        }
    } else if (CommandDelimiter == CHAR_EXEC_MODE) {
        CommandExecFuncType ExecFunc = pgm_read_ptr(&CommandEntry->ExecFunc);
        if (ExecFunc != NO_FUNCTION) {
            Status = ExecFunc(pTerminalBuffer);
        }
    } else if (CommandDelimiter == CHAR_EXEC_MODE_PARAM) {
        CommandExecParamFuncType ExecParamFunc = pgm_read_ptr(&CommandEntry->ExecParamFunc);
        if (ExecParamFunc != NO_FUNCTION) {
            Status = ExecParamFunc(pTerminalBuffer, pParam);
        }
    } else {
        /* This should not happen (TM) */
    }

    if (Status == TIMEOUT_COMMAND) {
        TaskPending = true;
        TaskPendingSince = SystemGetSysTick();
    }

    /* This delimiter has not been registered with this command */
    return Status;
}

void CommandExecute(const char *command) {
    uint8_t i;

    for (i = 0; i < ARRAY_COUNT(CommandTable); i++) {
        if (strcmp_P(command, CommandTable[i].Command) == 0) {
            CallCommandFunc(&CommandTable[i], CHAR_EXEC_MODE, NULL);
            break;
        }
    }
}

static void DecodeCommand(void) {
    uint8_t i;
    bool CommandFound = false;
    CommandStatusIdType StatusId = COMMAND_ERR_UNKNOWN_CMD_ID;
    char *pTerminalBuffer = (char *) TerminalBuffer;

    /* Do some sanity check first */
    if (!IS_COMMAND_DELIMITER(pTerminalBuffer[0])) {
        char *pCommandDelimiter = pTerminalBuffer;
        char CommandDelimiter = '\0';

        /* Search for command delimiter, store it and replace with '\0' */
        while (!(IS_COMMAND_DELIMITER(*pCommandDelimiter)))
            pCommandDelimiter++;

        CommandDelimiter = *pCommandDelimiter;
        *pCommandDelimiter = '\0';

        /* Search in command table */
        for (i = 0; i < ARRAY_COUNT(CommandTable); i++) {
            if (strcmp_P(pTerminalBuffer, CommandTable[i].Command) == 0) {
                /* Command found. Clear buffer, and call appropriate function */
                char *pParam = ++pCommandDelimiter;

                pTerminalBuffer[0] = '\0';
                CommandFound = true;

                StatusId = CallCommandFunc(&CommandTable[i], CommandDelimiter, pParam);

                break;
            }
        }
    }

    if (StatusId == TIMEOUT_COMMAND) // it is a timeout command, so we return
        return;

    /* Send command status message */
    TerminalSendStringP(GetStatusMessageP(StatusId));
    TerminalSendStringP(PSTR(STATUS_MESSAGE_TRAILER));

    if (CommandFound && (pTerminalBuffer[0] != '\0')) {
        /* Send optional answer */
        TerminalSendString(pTerminalBuffer);
        TerminalSendStringP(PSTR(OPTIONAL_ANSWER_TRAILER));
        if (StringLength(pTerminalBuffer, TERMINAL_BUFFER_SIZE) + 1 >= TERMINAL_BUFFER_SIZE) {
            /*
             * Notify the user that the command line output is truncated. This can come up in the
             * 'CONFIG=MF_DESFIRE' variants where the Makefile setting 'MEMORY_LIMITED_TESTING' is
             * enabled by default to save space for other necessary components.
             */
            TerminalSendStringP(PSTR("--TRUNCATED OUTPUT--"));
            TerminalSendStringP(PSTR(OPTIONAL_ANSWER_TRAILER));
        }
    }
}

void CommandLineInit(void) {
    TerminalBufferIdx = 0;
}

bool CommandLineProcessByte(uint8_t Byte) {
    if (IS_CHARACTER(Byte)) {
        /* Store uppercase character */
        if (IS_LOWERCASE(Byte)) {
            Byte = TO_UPPERCASE(Byte);
        }

        /* Prevent buffer overflow and account for '\0' */
        if (TerminalBufferIdx < TERMINAL_BUFFER_SIZE - 1) {
            TerminalBuffer[TerminalBufferIdx++] = Byte;
        }
    } else if (Byte == '\r') {
        /* Process on \r. Terminate string and decode. */
        TerminalBuffer[TerminalBufferIdx] = '\0';
        TerminalBufferIdx = 0;

        if (!TaskPending)
            DecodeCommand();
    } else if (Byte == '\b') {
        /* Backspace. Delete last character in buffer. */
        if (TerminalBufferIdx > 0) {
            TerminalBufferIdx--;
        }
    } else if (Byte == 0x1B) {
        /* Drop buffer on escape */
        TerminalBufferIdx = 0;
    } else {
        /* Ignore other chars */
    }

    return true;
}

INLINE void Timeout(void) {
    TaskPending = false;
    TerminalSendStringP(GetStatusMessageP(COMMAND_ERR_TIMEOUT_ID));
    TerminalSendStringP(PSTR(STATUS_MESSAGE_TRAILER));

    if (CommandLinePendingTaskTimeout != NO_FUNCTION) {
        CommandLinePendingTaskTimeout(); // call the function that ends the task
        CommandLinePendingTaskTimeout = NO_FUNCTION;
    }
}

void CommandLineTick(void) {
    if (TaskPending &&
            GlobalSettings.ActiveSettingPtr->PendingTaskTimeout != 0 && // 0 means no timeout
            SYSTICK_DIFF_100MS(TaskPendingSince) >= GlobalSettings.ActiveSettingPtr->PendingTaskTimeout) { // timeout expired
        Timeout();
    }
}

void CommandLinePendingTaskBreak(void) {
    if (!TaskPending)
        return;

    Timeout();
}

void CommandLinePendingTaskFinished(CommandStatusIdType ReturnStatusID, char const *const OutMessage) {
    if (!TaskPending) // if no task is pending, no task can be finished
        return;
    TaskPending = false;

    TerminalSendStringP(GetStatusMessageP(ReturnStatusID));
    TerminalSendStringP(PSTR(STATUS_MESSAGE_TRAILER));

    if (OutMessage != NULL) {
        TerminalSendString(OutMessage);
        TerminalSendStringP(PSTR(OPTIONAL_ANSWER_TRAILER));
    }
}

void CommandLineAppendData(void const *const Buffer, uint16_t Bytes) {
    char *pTerminalBuffer = (char *) TerminalBuffer;

    uint16_t tmpBytes = Bytes;
    if (Bytes > (TERMINAL_BUFFER_SIZE / 2))
        tmpBytes = TERMINAL_BUFFER_SIZE / 2;
    Bytes -= tmpBytes;

    BufferToHexString(pTerminalBuffer, TERMINAL_BUFFER_SIZE, Buffer, tmpBytes);
    TerminalSendString(pTerminalBuffer);

    uint8_t i = 1;
    while (Bytes > (TERMINAL_BUFFER_SIZE / 2)) {
        Bytes -= TERMINAL_BUFFER_SIZE / 2;
        BufferToHexString(pTerminalBuffer, TERMINAL_BUFFER_SIZE, Buffer + i * TERMINAL_BUFFER_SIZE / 2, TERMINAL_BUFFER_SIZE);
        TerminalSendString(pTerminalBuffer);
        i++;
    }

    if (Bytes > 0) {
        BufferToHexString(pTerminalBuffer, TERMINAL_BUFFER_SIZE, Buffer + i * TERMINAL_BUFFER_SIZE / 2, Bytes);
        TerminalSendString(pTerminalBuffer);
    }

    TerminalSendStringP(PSTR(OPTIONAL_ANSWER_TRAILER));
}
