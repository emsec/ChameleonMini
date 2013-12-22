/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
 */

#include "CommandLine.h"

#define CHAR_GET_MODE   '?'     /* <Command>? */
#define CHAR_SET_MODE   '='     /* <Command>=<Param> */
#define CHAR_EXEC_MODE  '\0'    /* <Command> */

#define IS_COMMAND_DELIMITER(c) ( \
  ((c) == CHAR_EXEC_MODE) || ((c) == CHAR_GET_MODE) || ((c) == CHAR_SET_MODE) \
)

#define IS_CHARACTER(c) ( \
  ( ((c) >= 'A') && ((c) <= 'Z') ) || \
  ( ((c) >= 'a') && ((c) <= 'z') ) || \
  ( ((c) >= '0') && ((c) <= '9') ) || \
  ( ((c) == '_') ) || \
  ( ((c) == CHAR_GET_MODE) || ((c) == CHAR_SET_MODE) ) \
)

#define IS_LOWERCASE(c) ( ((c) >= 'a') && ((c) <= 'z') )
#define TO_UPPERCASE(c) ( (c) - 'a' + 'A' )

#define NO_FUNCTION    ((void*) 0)

#define STATUS_MESSAGE_TRAILER    "\r\n"
#define OPTIONAL_ANSWER_TRAILER    "\r\n"

/* Include all command functions */
#include "Commands.h"

const PROGMEM CommandEntryType CommandTable[] = {
  {
    .Command    = COMMAND_VERSION,
    .ExecFunc   = NO_FUNCTION,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = CommandGetVersion,
  },
  {
    .Command    = COMMAND_CONFIG,
    .ExecFunc   = CommandExecConfig,
    .SetFunc    = CommandSetConfig,
    .GetFunc    = CommandGetConfig
  },
  {
    .Command    = COMMAND_UID,
    .ExecFunc   = NO_FUNCTION,
    .SetFunc    = CommandSetUid,
    .GetFunc    = CommandGetUid
  },
  {
    .Command    = COMMAND_READONLY,
    .ExecFunc   = NO_FUNCTION,
    .GetFunc    = CommandGetReadOnly,
    .SetFunc    = CommandSetReadOnly

  },
  {
    .Command    = COMMAND_UPLOAD,
    .ExecFunc   = CommandExecUpload,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = NO_FUNCTION
  },
  {
    .Command    = COMMAND_DOWNLOAD,
    .ExecFunc   = CommandExecDownload,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = NO_FUNCTION
  },
  {
    .Command    = COMMAND_RESET,
    .ExecFunc   = CommandExecReset,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = NO_FUNCTION
  },
#ifdef SUPPORT_FIRMWARE_UPGRADE
  {
    .Command    = COMMAND_UPGRADE,
    .ExecFunc   = CommandExecUpgrade,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = NO_FUNCTION
  },
#endif
  {
    .Command    = COMMAND_MEMSIZE,
    .ExecFunc   = NO_FUNCTION,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = CommandGetMemSize
  },
  {
    .Command    = COMMAND_UIDSIZE,
    .ExecFunc   = NO_FUNCTION,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = CommandGetUidSize
  },
  {
    .Command    = COMMAND_BUTTON,
    .ExecFunc   = CommandExecButton,
    .SetFunc    = CommandSetButton,
    .GetFunc    = CommandGetButton
  },
  {
	.Command	= COMMAND_SETTING,
	.ExecFunc	= NO_FUNCTION,
	.SetFunc	= CommandSetSetting,
	.GetFunc	= CommandGetSetting
  },
  {
	.Command	= COMMAND_CLEAR,
	.ExecFunc	= CommandExecClear,
	.SetFunc	= NO_FUNCTION,
	.GetFunc	= NO_FUNCTION
  },
  {
    .Command    = COMMAND_HELP,
    .ExecFunc   = CommandExecHelp,
    .SetFunc    = NO_FUNCTION,
    .GetFunc    = NO_FUNCTION
  },
  {
	.Command	= COMMAND_RSSI,
	.ExecFunc 	= NO_FUNCTION,
	.SetFunc 	= NO_FUNCTION,
	.GetFunc 	= CommandGetRssi
  },


  { /* This has to be last element */
    .Command    = COMMAND_LIST_END,
    .ExecFunc   = NO_FUNCTION,
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
};

static uint16_t BufferIdx;

static const char* GetStatusMessageP(CommandStatusIdType StatusId)
{
    uint8_t i;

    for (i=0; i<(sizeof(StatusTable)/sizeof(*StatusTable)); i++) {
        if (pgm_read_byte(&StatusTable[i].Id) == StatusId)
            return StatusTable[i].Message;
    }

    return (void*) 0;
}

static CommandStatusIdType CallCommandFunc(
    const CommandEntryType* CommandEntry, char CommandDelimiter, char* pParam) {
  char* pTerminalBuffer = (char*) TerminalBuffer;

  /* Call appropriate function depending on CommandDelimiter */
  if (CommandDelimiter == CHAR_GET_MODE) {
    CommandGetFuncType GetFunc = pgm_read_ptr(&CommandEntry->GetFunc);
    if (GetFunc != NO_FUNCTION) {
      return GetFunc(pTerminalBuffer);
    }
  } else if (CommandDelimiter == CHAR_SET_MODE) {
    CommandSetFuncType SetFunc = pgm_read_ptr(&CommandEntry->SetFunc);
    if (SetFunc != NO_FUNCTION) {
      return SetFunc(pParam);
    }
  } else if (CommandDelimiter == CHAR_EXEC_MODE){
    CommandExecFuncType ExecFunc = pgm_read_ptr(&CommandEntry->ExecFunc);
    if (ExecFunc != NO_FUNCTION) {
      return ExecFunc(pTerminalBuffer);
    }
  } else {
    /* This should not happen (TM) */
  }

  /* This delimiter has not been registered with this command */
  return COMMAND_ERR_INVALID_USAGE_ID;
}

static void DecodeCommand(void)
{
  uint8_t i;
  bool CommandFound = false;
  CommandStatusIdType StatusId = COMMAND_ERR_UNKNOWN_CMD_ID;
  char* pTerminalBuffer = (char*) TerminalBuffer;

  /* Do some sanity check first */
  if (!IS_COMMAND_DELIMITER(pTerminalBuffer[0])) {
    char* pCommandDelimiter = pTerminalBuffer;
    char CommandDelimiter = '\0';

    /* Search for command delimiter, store it and replace with '\0' */
    while(!(IS_COMMAND_DELIMITER(*pCommandDelimiter)))
      pCommandDelimiter++;

    CommandDelimiter = *pCommandDelimiter;
    *pCommandDelimiter = '\0';

    /* Search in command table */
    for (i=0; i<(sizeof(CommandTable) / sizeof(*CommandTable)); i++) {
      if (strcmp_P(pTerminalBuffer, CommandTable[i].Command) == 0) {
        /* Command found. Clear buffer, and call appropriate function */
        char* pParam = ++pCommandDelimiter;

        pTerminalBuffer[0] = '\0';
        CommandFound = true;

        StatusId = CallCommandFunc(&CommandTable[i], CommandDelimiter, pParam);

        break;
      }
    }
  }

  /* Send command status message */
  TerminalSendStringP(GetStatusMessageP(StatusId));
  TerminalSendStringP(PSTR(STATUS_MESSAGE_TRAILER));

  if (CommandFound && (pTerminalBuffer[0] != '\0') ) {
    /* Send optional answer */
    TerminalSendString(pTerminalBuffer);
    TerminalSendStringP(PSTR(OPTIONAL_ANSWER_TRAILER));
  }
}

void CommandLineInit(void)
{
  BufferIdx = 0;
}

bool CommandLineProcessByte(uint8_t Byte) {
  if (IS_CHARACTER(Byte)){
    /* Store uppercase character */
    if (IS_LOWERCASE(Byte)) {
      Byte = TO_UPPERCASE(Byte);
    }

    /* Prevent buffer overflow and account for '\0' */
    if (BufferIdx < (sizeof(TerminalBuffer) / sizeof(*TerminalBuffer) - 1)) {
      TerminalBuffer[BufferIdx++] = Byte;
    }
  }else if (Byte == '\r') {
    /* Process on \r. Terminate string and decode. */
    TerminalBuffer[BufferIdx] = '\0';
    BufferIdx = 0;

    DecodeCommand();
  }else if (Byte == '\b') {
    /* Backspace. Delete last character in buffer. */
    if (BufferIdx > 0) {
      BufferIdx--;
    }
  } else if (Byte == 0x1B){
    /* Drop buffer on escape */
    BufferIdx = 0;
  } else {
    /* Ignore other chars */
  }

  return true;
}

void CommandLineTick(void)
{

}
