
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
  snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,
    PSTR("%s"), ActiveConfiguration.ConfigurationName);

  return COMMAND_INFO_OK_WITH_TEXT_ID;

}

CommandStatusIdType CommandSetConfig(const char* InParam)
{
  if (ConfigurationSetByName(InParam)) {
	    SettingsSave();
    return COMMAND_INFO_OK_ID;
  } else {
    return COMMAND_ERR_INVALID_PARAM_ID;
  }
}

CommandStatusIdType CommandExecConfig(char* OutMessage)
{
  ConfigurationGetList(OutMessage, TERMINAL_BUFFER_SIZE);

  return COMMAND_INFO_OK_WITH_TEXT_ID;
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

CommandStatusIdType CommandSetUid(const char* InParam)
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

CommandStatusIdType CommandSetReadOnly(const char* InParam)
{
  if (InParam[1] == '\0') {
    if (InParam[0] == COMMAND_CHAR_TRUE) {
      ActiveConfiguration.ReadOnly = true;
      return COMMAND_INFO_OK_ID;
    } else if (InParam[0] == COMMAND_CHAR_FALSE) {
      ActiveConfiguration.ReadOnly = false;
      return COMMAND_INFO_OK_ID;
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

CommandStatusIdType CommandExecButton(char* OutMessage)
{
    ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetButton(char* OutParam)
{
    ButtonGetActionByName(BUTTON_PRESS_SHORT, OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetButton(const char* InParam)
{
    if (ButtonSetActionByName(BUTTON_PRESS_SHORT, InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandExecButtonLong(char* OutMessage)
{
    ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetButtonLong(char* OutParam)
{
    ButtonGetActionByName(BUTTON_PRESS_LONG, OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetButtonLong(const char* InParam)
{
    if (ButtonSetActionByName(BUTTON_PRESS_LONG, InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandExecLedGreen(char* OutMessage)
{
	LEDGetFuncList(OutMessage, TERMINAL_BUFFER_SIZE);

	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetLedGreen(char* OutParam)
{
	LEDGetFuncByName(LED_GREEN, OutParam, TERMINAL_BUFFER_SIZE);

	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLedGreen(const char* InParam)
{
	if (LEDSetFuncByName(LED_GREEN, InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandExecLedRed(char* OutMessage)
{
	LEDGetFuncList(OutMessage, TERMINAL_BUFFER_SIZE);

	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetLedRed(char* OutParam)
{
	LEDGetFuncByName(LED_RED, OutParam, TERMINAL_BUFFER_SIZE);

	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLedRed(const char* InParam)
{
	if (LEDSetFuncByName(LED_RED, InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandExecLogMode(char* OutMessage)
{
    /* Get list of log modes */
    LogGetModeList(OutMessage, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetLogMode(char* OutParam)
{
    /* Get Logmode */
    LogGetModeByName(OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLogMode(const char* InParam)
{
    if (LogSetModeByName(InParam)) {
        SettingsSave();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandExecLogMem(char* OutMessage)
{
    snprintf_P(OutMessage, TERMINAL_BUFFER_SIZE,
        PSTR("%S,%S"), PSTR(COMMAND_LOGMEM_LOADBIN), PSTR(COMMAND_LOGMEM_CLEAR) );

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetLogMem(char* OutParam)
{
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,
        PSTR("%u"), LogMemFree());


    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetLogMem(const char* InParam)
{
    if (strcmp_P(InParam, PSTR(COMMAND_LOGMEM_LOADBIN)) == 0) {
        XModemSend(LogMemLoadBlock);
        return COMMAND_INFO_XMODEM_WAIT_ID;
    } else if (strcmp_P(InParam, PSTR(COMMAND_LOGMEM_CLEAR)) == 0) {
        LogMemClear();
        return COMMAND_INFO_OK_ID;
    } else {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
}

CommandStatusIdType CommandGetSetting(char* OutParam)
{
	SettingsGetActiveByName(OutParam, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetSetting(const char* InParam)
{
	if (SettingsSetActiveByName(InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandExecClear(char* OutParam)
{
	MemoryClear();
	return COMMAND_INFO_OK_ID;
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
