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
    .GetFunc        = NO_FUNCTION
}, {
    .Command        = DFCOMMAND_COMM_MODE,
    .ExecFunc       = NO_FUNCTION,
    .ExecParamFunc  = NO_FUNCTION,
    .SetFunc        = CommandDESFireSetCommMode,
    .GetFunc        = NO_FUNCTION
}, {
    .Command        = DFCOMMAND_SET_ENCMODE,
    .ExecFunc       = NO_FUNCTION,
    .ExecParamFunc  = NO_FUNCTION,
    .SetFunc        = CommandDESFireSetEncryptionMode,
    .GetFunc        = NO_FUNCTION
},

#endif

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
