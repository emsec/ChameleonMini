/*
The DESFire stack portion of this firmware source
is free software written by Maxie Dion Schmidt (@maxieds):
You can redistribute it and/or modify
it under the terms of this license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

The complete source distribution of
this firmware is available at the following link:
https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.

Based in part on the original DESFire code created by
@dev-zzo (GitHub handle) [Dmitry Janushkevich] available at
https://github.com/dev-zzo/ChameleonMini/tree/desfire.

This notice must be retained at the top of all source files where indicated.
*/

/*
 * DESFireChameleonTerminal.h
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_CHAMELEON_TERMINAL_H__
#define __DESFIRE_CHAMELEON_TERMINAL_H__

#if defined(CONFIG_MF_DESFIRE_SUPPORT) && !defined(DISABLE_DESFIRE_TERMINAL_COMMANDS)

#include <stdbool.h>

#include "../../Terminal/Commands.h"
#include "../../Terminal/CommandLine.h"

bool IsDESFireConfiguration(void);

#ifndef DISABLE_PERMISSIVE_DESFIRE_SETTINGS
#define DFCOMMAND_SET_HEADER                  "DF_SETHDR"
CommandStatusIdType CommandDESFireGetHeaderProperty(char *OutParam);
CommandStatusIdType CommandDESFireSetHeaderProperty(char *OutMessage, const char *InParams);
#endif

#define DFCOMMAND_LAYOUT_PPRINT               "DF_PPRINT_PICC"
CommandStatusIdType CommandDESFireLayoutPPrint(char *OutParam, const char *InParams);

#define DFCOMMAND_FIRMWARE_INFO               "DF_FWINFO"
CommandStatusIdType CommandDESFireFirmwareInfo(char *OutParam);

#define DFCOMMAND_LOGGING_MODE                "DF_LOGMODE"
CommandStatusIdType CommandDESFireGetLoggingMode(char *OutParam);
CommandStatusIdType CommandDESFireSetLoggingMode(char *OutMessage, const char *InParams);

#define DFCOMMAND_TESTING_MODE                "DF_TESTMODE"
CommandStatusIdType CommandDESFireGetTestingMode(char *OutParam);
CommandStatusIdType CommandDESFireSetTestingMode(char *OutMessage, const char *InParams);

#endif

#endif
