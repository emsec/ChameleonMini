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
    ButtonGetActionByName(OutParam, TERMINAL_BUFFER_SIZE);

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetButton(const char* InParam)
{
    if (ButtonSetActionByName(InParam)) {
        SettingsSave();
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
