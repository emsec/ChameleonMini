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
 *
 * Part of this file was added by Tomas Preucil (github.com/tomaspre)
 * This part is indicated in the code below
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
CommandStatusIdType CommandDESFireSetHeaderProperty(char *OutMessage, const char *InParams);
#endif /* DISABLE_PERMISSIVE_DESFIRE_SETTINGS */

#define DFCOMMAND_COMM_MODE                   "DF_COMM_MODE"
CommandStatusIdType CommandDESFireSetCommMode(char *OutMessage, const char *InParams);

#define DFCOMMAND_SET_ENCMODE                 "DF_ENCMODE"
CommandStatusIdType CommandDESFireSetEncryptionMode(char *OutMessage, const char *InParams);

//The rest of the file was added by tomaspre
#define DFCOMMAND_SETUP_GALLAGHER                      "DF_SETUP_GALL"
CommandStatusIdType CommandDESFireSetupGallagher(char *OutMessage, const char *InParams);

#define DFCOMMAND_CREATE_GALLAGHER_APP                 "DF_CRE_GALLAPP"
CommandStatusIdType CommandDESFireCreateGallagherApp(char *OutMessage, const char *InParams);

#define DFCOMMAND_UPDATE_GALLAGHER_APP                 "DF_UP_GALLAPP"
CommandStatusIdType CommandDESFireUpdateGallagherApp(char *OutMessage, const char *InParams);

#define DFCOMMAND_UPDATE_GALLAGHER_CARD_ID             "DF_UP_GALL_CID"
CommandStatusIdType CommandDESFireUpdateGallagherCardId(char *OutMessage, const char *InParams);

#define DFCOMMAND_SELECT_GALLAGHER_APP                 "DF_SEL_GALLAPP"
CommandStatusIdType CommandDESFireSelectGallagherApp(char *OutMessage, const char *InParams);

#define DFCOMMAND_SET_GALLAGHER_SITE_KEY               "DF_SET_GALLKEY"
CommandStatusIdType CommandDESFireSetGallagherSiteKey(char *OutMessage, const char *InParams);

#endif /* DESFire Support */

#endif /* __DESFIRE_CHAMELEON_TERMINAL_H__ */
