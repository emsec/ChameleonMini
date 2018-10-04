
#include "Commands.h"
#include <stdio.h>
#include <avr/pgmspace.h>
#include "XModem.h"
#include "../Settings.h"
#include "../Chameleon-Mini.h"
#include "../LUFA/Version.h"
#include "../Configuration.h"
#include "../Random.h"
#include "../Memory.h"
#include "../System.h"
#include "../Button.h"
#include "../AntennaLevel.h"
#include "../Battery.h"

extern const PROGMEM CommandEntryType CommandTable[];

CommandStatusIdType CommandGetVersion(char* OutParam)
{
  snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR(
    "Chameleon-Mini %S using LUFA %S compiled with AVR-GCC %S"
    ), PSTR(CHAMELEON_MINI_VERSION_STRING), PSTR(LUFA_VERSION_STRING), PSTR(__VERSION__)
  );

  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetConfig(char* OutParam)
{
  ConfigurationGetByName(OutParam, TERMINAL_BUFFER_SIZE);

  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetConfig(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		ConfigurationGetList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (ConfigurationSetByName(InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandGetUid(char* OutParam)
{
  uint8_t UidBuffer[COMMAND_UID_BUFSIZE];
  uint16_t UidSize = ActiveConfiguration.UidSize;

  ApplicationGetUid(UidBuffer);

  BufferToHexString(OutParam, TERMINAL_BUFFER_SIZE,
    UidBuffer, UidSize);

  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetUid(char* OutMessage, const char* InParam)
{
  uint8_t UidBuffer[COMMAND_UID_BUFSIZE];
  uint16_t UidSize = ActiveConfiguration.UidSize;

  if (strcmp_P(InParam, PSTR(COMMAND_UID_RANDOM)) == 0) {
    /* Load with random bytes */
    for (uint8_t i=0; i<UidSize; i++) {
      UidBuffer[i] = RandomGetByte();
    }
  } else {
    /* Convert to Bytes */
    if (HexStringToBuffer(UidBuffer, sizeof(UidBuffer), InParam) != UidSize) {
      /* Malformed input. Abort */
      return COMMAND_ERR_INVALID_PARAM_ID;
    }
  }

  ApplicationSetUid(UidBuffer);

  return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandGetReadOnly(char* OutParam)
{
  if (ActiveConfiguration.ReadOnly) {
    OutParam[0] = COMMAND_CHAR_TRUE;
  } else {
    OutParam[0] = COMMAND_CHAR_FALSE;
  }

  OutParam[1] = '\0';

  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetReadOnly(char* OutMessage, const char* InParam)
{
  if (InParam[1] == '\0') {
    if (InParam[0] == COMMAND_CHAR_TRUE) {
      ActiveConfiguration.ReadOnly = true;
      return COMMAND_INFO_OK_ID;
    } else if (InParam[0] == COMMAND_CHAR_FALSE) {
      ActiveConfiguration.ReadOnly = false;
      return COMMAND_INFO_OK_ID;
    } else if (InParam[0] == COMMAND_CHAR_SUGGEST) {
    	snprintf_P(OutMessage, TERMINAL_BUFFER_SIZE, PSTR("%c,%c"), COMMAND_CHAR_TRUE, COMMAND_CHAR_FALSE);
    	return COMMAND_INFO_OK_WITH_TEXT_ID;
    }
  }

  return COMMAND_ERR_INVALID_PARAM_ID;
}

CommandStatusIdType CommandExecUpload(char* OutMessage)
{
    XModemReceive(MemoryUploadBlock);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecDownload(char* OutMessage)
{
    XModemSend(MemoryDownloadBlock);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecReset(char* OutMessage)
{
  USB_Detach();
  USB_Disable();

  SystemReset();

  return COMMAND_INFO_OK_ID;
}

#ifdef SUPPORT_FIRMWARE_UPGRADE
CommandStatusIdType CommandExecUpgrade(char* OutMessage)
{
  USB_Detach();
  USB_Disable();

  SystemEnterBootloader();

  return COMMAND_INFO_OK_ID;
}
#endif

CommandStatusIdType CommandGetMemSize(char* OutParam)
{
  snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%u"), ActiveConfiguration.MemorySize);

  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetUidSize(char* OutParam)
{
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%u"), ActiveConfiguration.UidSize);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetRButton(char* OutParam)
{
    ButtonGetActionByName(BUTTON_R_PRESS_SHORT, OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetRButton(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		/* Get suggestion list */
		ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (ButtonSetActionByName(BUTTON_R_PRESS_SHORT, InParam)) {
		/* Try to set action name */
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandGetRButtonLong(char* OutParam)
{
    ButtonGetActionByName(BUTTON_R_PRESS_LONG, OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetRButtonLong(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		/* Get suggestion list */
		ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (ButtonSetActionByName(BUTTON_R_PRESS_LONG, InParam)) {
		/* Try to set action name */
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandGetLButton(char* OutParam)
{
    ButtonGetActionByName(BUTTON_L_PRESS_SHORT, OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLButton(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		/* Get suggestion list */
		ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (ButtonSetActionByName(BUTTON_L_PRESS_SHORT, InParam)) {
		/* Try to set action name */
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandGetLButtonLong(char* OutParam)
{
    ButtonGetActionByName(BUTTON_L_PRESS_LONG, OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLButtonLong(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		/* Get suggestion list */
		ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (ButtonSetActionByName(BUTTON_L_PRESS_LONG, InParam)) {
		/* Try to set action name */
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandGetLedGreen(char* OutParam)
{
	LEDGetFuncByName(LED_GREEN, OutParam, TERMINAL_BUFFER_SIZE);

	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLedGreen(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		LEDGetFuncList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (LEDSetFuncByName(LED_GREEN, InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandGetLedRed(char* OutParam)
{
	LEDGetFuncByName(LED_RED, OutParam, TERMINAL_BUFFER_SIZE);

	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLedRed(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		LEDGetFuncList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (LEDSetFuncByName(LED_RED, InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandGetLogMode(char* OutParam)
{
    /* Get Logmode */
    LogGetModeByName(OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLogMode(char* OutMessage, const char* InParam)
{
	if (COMMAND_IS_SUGGEST_STRING(InParam)) {
		LogGetModeList(OutMessage, TERMINAL_BUFFER_SIZE);
		return COMMAND_INFO_OK_WITH_TEXT_ID;
	} else if (LogSetModeByName(InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandGetLogMem(char* OutParam)
{
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,
        PSTR("%u"), LogMemFree());


    return COMMAND_INFO_OK_WITH_TEXT_ID;
}


CommandStatusIdType CommandExecLogDownload(char* OutMessage)
{
    XModemSend(LogMemLoadBlock);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecLogClear(char* OutMessage)
{
    LogMemClear();
    return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandGetSetting(char* OutParam)
{
	SettingsGetActiveByName(OutParam, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetSetting(char* OutMessage, const char* InParam)
{
	if (SettingsSetActiveByName(InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandExecClear(char* OutMessage)
{
	MemoryClear();
	return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandExecStore(char* OutMessage)
{
	MemoryStore();
	return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandExecRecall(char* OutMessage)
{
	MemoryRecall();
	return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandGetCharging(char* OutMessage)
{
	if (BatteryIsCharging()) {
		return COMMAND_INFO_TRUE_ID;
	} else {
		return COMMAND_INFO_FALSE_ID;
	}
}

CommandStatusIdType CommandExecHelp(char* OutMessage)
{
    const CommandEntryType* EntryPtr = CommandTable;
    uint16_t ByteCount = TERMINAL_BUFFER_SIZE - 1; /* Account for '\0' */

    while(strcmp_P(COMMAND_LIST_END, EntryPtr->Command) != 0) {
        const char* CommandName = EntryPtr->Command;
        char c;

        while( (c = pgm_read_byte(CommandName)) != '\0' && ByteCount > 32) {
            *OutMessage++ = c;
            CommandName++;
            ByteCount--;
        }

        *OutMessage++ = ',';
        ByteCount--;

        EntryPtr++;
    }

    *--OutMessage = '\0';

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetRssi(char* OutParam)
{
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,
        PSTR("%5u mV"), AntennaLevelGet());

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetSysTick(char* OutParam)
{
	snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%4.4X"), SystemGetSysTick());

	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

