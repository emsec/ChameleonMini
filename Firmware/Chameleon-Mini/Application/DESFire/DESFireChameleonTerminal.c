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
 * DESFireChameleonTerminal.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#if defined(CONFIG_MF_DESFIRE_SUPPORT) && !defined(DISABLE_DESFIRE_TERMINAL_COMMANDS)

#include "../../Terminal/Terminal.h"
#include "../../Terminal/Commands.h"
#include "../../Settings.h"
#include "DESFireChameleonTerminal.h"
#include "DESFireFirmwareSettings.h"
#include "DESFirePICCControl.h"
#include "DESFireMemoryOperations.h"
#include "DESFireLogging.h"

bool IsDESFireConfiguration(void) {
    return GlobalSettings.ActiveSettingPtr->Configuration == CONFIG_MF_DESFIRE ||
           GlobalSettings.ActiveSettingPtr->Configuration == CONFIG_MF_DESFIRE_2KEV1 ||
           GlobalSettings.ActiveSettingPtr->Configuration == CONFIG_MF_DESFIRE_4KEV1 ||
           GlobalSettings.ActiveSettingPtr->Configuration == CONFIG_MF_DESFIRE_4KEV2;
}

#ifndef DISABLE_PERMISSIVE_DESFIRE_SETTINGS
CommandStatusIdType CommandDESFireSetHeaderProperty(char *OutParam, const char *InParams) {
    if (!IsDESFireConfiguration()) {
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
    char hdrPropSpecStr[24];
    char propSpecBytesStr[16];
    BYTE propSpecBytes[16];
    SIZET dataByteCount = 0x00;
    BYTE StatusError = 0x00;
    if (!sscanf_P(InParams, PSTR("%24s %12s"), hdrPropSpecStr, propSpecBytesStr)) {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
    hdrPropSpecStr[23] = propSpecBytesStr[15] = '\0';
    dataByteCount = HexStringToBuffer(propSpecBytes, 16, propSpecBytesStr);
    if (!strcasecmp_P(hdrPropSpecStr, PSTR("ATS"))) {
        if (dataByteCount != 5) {
            StatusError = 1;
        } else {
            memcpy(&Picc.ATSBytes[0], propSpecBytes, dataByteCount);
        }
    }
    if (!strcasecmp_P(hdrPropSpecStr, PSTR("ATQA"))) {
        if (dataByteCount != 2) {
            StatusError = 1;
        } else {
            Picc.ATQA[0] = propSpecBytes[0];
            Picc.ATQA[1] = propSpecBytes[1];
            DesfireATQAReset = true;
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("ManuID"))) {
        if (dataByteCount != 1) {
            StatusError = 1;
        } else {
            Picc.ManufacturerID = propSpecBytes[0];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("HwType"))) {
        if (dataByteCount != 1) {
            StatusError = 1;
        } else {
            Picc.HwType = propSpecBytes[0];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("HwSubtype"))) {
        if (dataByteCount != 1) {
            StatusError = 1;
        } else {
            Picc.HwSubtype = propSpecBytes[0];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("HWProtoType"))) {
        if (dataByteCount != 1) {
            StatusError = 1;
        } else {
            Picc.HwProtocolType = propSpecBytes[0];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("HWVers"))) {
        if (dataByteCount != 2) {
            StatusError = 1;
        } else {
            Picc.HwVersionMajor = propSpecBytes[0];
            Picc.HwVersionMinor = propSpecBytes[1];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("SwType"))) {
        if (dataByteCount != 1) {
            StatusError = 1;
        } else {
            Picc.SwType = propSpecBytes[0];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("SwSubtype"))) {
        if (dataByteCount != 1) {
            StatusError = 1;
        } else {
            Picc.SwSubtype = propSpecBytes[0];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("SwProtoType"))) {
        if (dataByteCount != 1) {
            StatusError = 1;
        } else {
            Picc.SwProtocolType = propSpecBytes[0];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("SwVers"))) {
        if (dataByteCount != 2) {
            StatusError = 1;
        } else {
            Picc.SwVersionMajor = propSpecBytes[0];
            Picc.SwVersionMinor = propSpecBytes[1];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("BatchNo"))) {
        if (dataByteCount != 5) {
            StatusError = 1;
        } else {
            memcpy(Picc.BatchNumber, propSpecBytes, 5);
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("ProdDate"))) {
        if (dataByteCount != 2) {
            StatusError = 1;
        } else {
            Picc.ProductionWeek = propSpecBytes[0];
            Picc.ProductionYear = propSpecBytes[1];
        }
    } else {
        StatusError = 1;
    }
    if (StatusError) {
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
    SynchronizePICCInfo();
    MemoryStoreDesfireHeaderBytes();
    return COMMAND_INFO_OK_ID;
}
#endif /* DISABLE_PERMISSIVE_DESFIRE_SETTINGS */

CommandStatusIdType CommandDESFireSetCommMode(char *OutParam, const char *InParams) {
    if (!IsDESFireConfiguration()) {
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
    char valueStr[16];
    if (!sscanf_P(InParams, PSTR("%15s"), valueStr)) {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
    valueStr[15] = '\0';
    if (!strcasecmp_P(valueStr, PSTR("Plaintext"))) {
        DesfireCommMode = DESFIRE_COMMS_PLAINTEXT;
        DesfireCommandState.ActiveCommMode = DesfireCommMode;
        return COMMAND_INFO_OK;
    } else if (!strcasecmp_P(valueStr, PSTR("Plaintext:MAC"))) {
        DesfireCommMode = DESFIRE_COMMS_PLAINTEXT_MAC;
        DesfireCommandState.ActiveCommMode = DesfireCommMode;
        return COMMAND_INFO_OK;
    } else if (!strcasecmp_P(valueStr, PSTR("Enciphered:3K3DES"))) {
        DesfireCommMode = DESFIRE_COMMS_CIPHERTEXT_DES;
        DesfireCommandState.ActiveCommMode = DesfireCommMode;
        return COMMAND_INFO_OK;
    } else if (!strcasecmp_P(valueStr, PSTR("Enciphered:AES128"))) {
        DesfireCommMode = DESFIRE_COMMS_CIPHERTEXT_AES128;
        DesfireCommandState.ActiveCommMode = DesfireCommMode;
        return COMMAND_INFO_OK;
    }
    return COMMAND_ERR_INVALID_USAGE_ID;
}

CommandStatusIdType CommandDESFireSetEncryptionMode(char *OutParam, const char *InParams) {
    if (!IsDESFireConfiguration()) {
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
    char valueStr[16];
    if (!sscanf_P(InParams, PSTR("%15s"), valueStr)) {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
    valueStr[15] = '\0';
    char *modeStartPos = strchr(valueStr, ':');
    bool setAESCryptoMode = true, setDESCryptoMode = true;
    bool ecbModeEnabled = true;
    if (modeStartPos == NULL) {
        modeStartPos = &valueStr;
    } else {
        uint8_t prefixLength = (uint8_t)(modeStartPos - valueStr);
        if (prefixLength == 0) {
            return COMMAND_ERR_INVALID_USAGE_ID;
        } else if (!strncasecmp_P(valueStr, PSTR("DES"), prefixLength)) {
            setAESCryptoMode = false;
        } else if (!strncasecmp_P(valueStr, PSTR("AES"), prefixLength)) {
            setDESCryptoMode = false;
        } else {
            return COMMAND_ERR_INVALID_USAGE_ID;
        }
    }
    if (!strcasecmp_P(modeStartPos, PSTR("ECB"))) {
        ecbModeEnabled = true;
    } else if (!strcasecmp_P(modeStartPos, PSTR("CBC"))) {
        ecbModeEnabled = false;
    } else {
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
    if (setDESCryptoMode) {
        __CryptoDESOpMode = ecbModeEnabled ? CRYPTO_DES_ECB_MODE : CRYPTO_DES_CBC_MODE;
    }
    if (setAESCryptoMode) {
        __CryptoAESOpMode = ecbModeEnabled ? CRYPTO_AES_ECB_MODE : CRYPTO_AES_CBC_MODE;
    }
    return COMMAND_INFO_OK;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
