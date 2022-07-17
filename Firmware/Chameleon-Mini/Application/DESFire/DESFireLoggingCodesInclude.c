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
 * DESFireLoggingCodesInclude.c :
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#ifndef __DESFIRE_LOGGING_CODES_INCLUDE_C__
#define __DESFIRE_LOGGING_CODES_INCLUDE_C__

LOG_ERR_DESFIRE_GENERIC_ERROR                = 0xE0,
LOG_INFO_DESFIRE_STATUS_INFO                 = 0xE1,
LOG_INFO_DESFIRE_DEBUGGING_OUTPUT            = 0xE2,
LOG_INFO_DESFIRE_INCOMING_DATA               = 0xE3,
LOG_INFO_DESFIRE_INCOMING_DATA_ENC           = 0xE4,
LOG_INFO_DESFIRE_OUTGOING_DATA               = 0xE5,
LOG_INFO_DESFIRE_OUTGOING_DATA_ENC           = 0xE6,
LOG_INFO_DESFIRE_NATIVE_COMMAND              = 0xE7,
LOG_INFO_DESFIRE_ISO1443_COMMAND             = 0xE8,
LOG_INFO_DESFIRE_ISO7816_COMMAND             = 0xE9,
LOG_INFO_DESFIRE_PICC_RESET                  = 0xEA,
LOG_INFO_DESFIRE_PICC_RESET_FROM_MEMORY      = 0XEB,
LOG_INFO_DESFIRE_PROTECTED_DATA_SET          = 0xEC,
LOG_INFO_DESFIRE_PROTECTED_DATA_SET_VERBOSE  = 0xED,

/* DESFire app */
LOG_APP_AUTH_KEY                             = 0xD0, ///< The key used for authentication
LOG_APP_NONCE_B                              = 0xD1, ///< Nonce B's value (generated)
LOG_APP_NONCE_AB                             = 0xD2, ///< Nonces A and B values (received)
LOG_APP_SESSION_IV                           = 0xD3, ///< Contents of the session IV buffer

/* ISO 14443 related entries */
LOG_ISO14443_3A_STATE                        = 0x53,
LOG_ISO14443_4_STATE                         = 0x54,
LOG_ISO14443A_APP_NO_RESPONSE                = 0x55,

#endif

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
