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
 * DESFireChameleonTerminalInclude.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#ifndef __DESFIRE_CHAMELEON_TERMINAL_INCLUDE_C__
#define __DESFIRE_CHAMELEON_TERMINAL_INCLUDE_C__
{
    .Command        = DFCOMMAND_SET_HEADER,
    .ExecFunc       = NO_FUNCTION,
    .ExecParamFunc  = NO_FUNCTION,
    .SetFunc        = CommandDESFireSetHeaderProperty,
    .GetFunc        = CommandDESFireGetHeaderProperty
}, {
    .Command        = DFCOMMAND_LAYOUT_PPRINT,
    .ExecFunc       = NO_FUNCTION,
    .ExecParamFunc  = CommandDESFireLayoutPPrint,
    .SetFunc        = NO_FUNCTION,
    .GetFunc        = NO_FUNCTION
}, {
    .Command        = DFCOMMAND_FIRMWARE_INFO,
    .ExecFunc       = CommandDESFireFirmwareInfo,
    .ExecParamFunc  = NO_FUNCTION,
    .SetFunc        = NO_FUNCTION,
    .GetFunc        = NO_FUNCTION
}, {
    .Command        = DFCOMMAND_LOGGING_MODE,
    .ExecFunc       = NO_FUNCTION,
    .ExecParamFunc  = NO_FUNCTION,
    .SetFunc        = CommandDESFireSetLoggingMode,
    .GetFunc        = CommandDESFireGetLoggingMode
}, {
    .Command        = DFCOMMAND_TESTING_MODE,
    .ExecFunc       = NO_FUNCTION,
    .ExecParamFunc  = NO_FUNCTION,
    .SetFunc        = CommandDESFireSetTestingMode,
    .GetFunc        = CommandDESFireGetTestingMode
},
#endif

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
