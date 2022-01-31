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
#include "DESFireLogging.h"

bool IsDESFireConfiguration(void) {
    return GlobalSettings.ActiveSettingPtr->Configuration == CONFIG_MF_DESFIRE;
}

CommandStatusIdType ExitOnInvalidConfigurationError(char *OutParam) {
    if (OutParam != NULL) {
        sprintf_P(OutParam, PSTR("Invalid Configuration: Set `CONFIG=MF_DESFIRE`.\r\n"));
    }
    return COMMAND_ERR_INVALID_USAGE_ID;
}

#ifndef DISABLE_PERMISSIVE_DESFIRE_SETTINGS
CommandStatusIdType CommandDESFireGetHeaderProperty(char *OutParam) {
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,
               PSTR("%s <ATS-5|HardwareVersion-2|SoftwareVersion-2|BatchNumber-5|ProductionDate-2> <HexBytes-N>"),
               DFCOMMAND_SET_HEADER);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandDESFireSetHeaderProperty(char *OutParam, const char *InParams) {
    if (!IsDESFireConfiguration()) {
        ExitOnInvalidConfigurationError(OutParam);
    }
    char hdrPropSpecStr[24];
    char propSpecBytesStr[16];
    BYTE propSpecBytes[16];
    SIZET dataByteCount = 0x00;
    BYTE StatusError = 0x00;
    if (!sscanf_P(InParams, PSTR("%24s %12s"), hdrPropSpecStr, propSpecBytesStr)) {
        CommandDESFireGetHeaderProperty(OutParam);
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
    hdrPropSpecStr[23] = propSpecBytesStr[15] = '\0';
    dataByteCount = HexStringToBuffer(propSpecBytes, 16, propSpecBytesStr);
    if (!strcasecmp_P(hdrPropSpecStr, PSTR("ATS"))) {
        if (dataByteCount != 5) {
            StatusError = 0x01;
        } else {
            memcpy(&Picc.ATSBytes[0], propSpecBytes, dataByteCount);
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("HardwareVersion"))) {
        if (dataByteCount != 2) {
            StatusError = 0x01;
        } else {
            Picc.HwVersionMajor = propSpecBytes[0];
            Picc.HwVersionMinor = propSpecBytes[1];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("SoftwareVersion"))) {
        if (dataByteCount != 2) {
            StatusError = 0x01;
        } else {
            Picc.SwVersionMajor = propSpecBytes[0];
            Picc.SwVersionMinor = propSpecBytes[1];
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("BatchNumber"))) {
        if (dataByteCount != 5) {
            StatusError = 0x01;
        } else {
            memcpy(Picc.BatchNumber, propSpecBytes, 5);
        }
    } else if (!strcasecmp_P(hdrPropSpecStr, PSTR("ProductionDate"))) {
        if (dataByteCount != 2) {
            StatusError = 0x01;
        } else {
            Picc.ProductionWeek = propSpecBytes[0];
            Picc.ProductionYear = propSpecBytes[1];
        }
    } else {
        StatusError = 0x01;
    }
    if (StatusError) {
        CommandDESFireGetHeaderProperty(OutParam);
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
    SynchronizePICCInfo();
    return COMMAND_INFO_OK_ID;
}
#endif /* DISABLE_PERMISSIVE_DESFIRE_SETTINGS */

CommandStatusIdType CommandDESFireLayoutPPrint(char *OutParam, const char *InParams) {
    if (!IsDESFireConfiguration()) {
        ExitOnInvalidConfigurationError(OutParam);
    }
    char pprintListSpecStr[32];
    BYTE StatusError = 0x00;
    if (!sscanf_P(InParams, PSTR("%31s"), pprintListSpecStr)) {
        StatusError = 0x01;
    } else {
        pprintListSpecStr[31] = '\0';
        if (!strcasecmp_P(pprintListSpecStr, PSTR("FullImage"))) {
            PrettyPrintPICCImageData((BYTE *) OutParam, TERMINAL_BUFFER_SIZE, 0x01);
        } else if (!strcasecmp_P(pprintListSpecStr, PSTR("HeaderData"))) {
            PrettyPrintPICCHeaderData((BYTE *) OutParam, TERMINAL_BUFFER_SIZE, 0x01);
        } else {
            StatusError = 0x01;
        }
    }
    if (StatusError) {
        snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,
                   PSTR("%s <FullImage|HeaderData>"),
                   DFCOMMAND_LAYOUT_PPRINT);
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandDESFireFirmwareInfo(char *OutParam) {
    if (!IsDESFireConfiguration()) {
        ExitOnInvalidConfigurationError(OutParam);
    }
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,
               PSTR("Chameleon-Mini DESFire enabled firmware built on %s "
                    "based on %s from \r\n"
                    "https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.\r\n"
                    "Revision: %s\r\n"),
               DESFIRE_FIRMWARE_BUILD_TIMESTAMP,
               DESFIRE_FIRMWARE_GIT_COMMIT_ID,
               DESFIRE_FIRMWARE_REVISION);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandDESFireGetLoggingMode(char *OutParam) {
    if (!IsDESFireConfiguration()) {
        ExitOnInvalidConfigurationError(OutParam);
    }
    switch (LocalLoggingMode) {
        case OFF:
            snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("OFF"));
            break;
        case NORMAL:
            snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("NORMAL"));
            break;
        case VERBOSE:
            snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("VERBOSE"));
            break;
        case DEBUGGING:
            snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("DEBUGGING"));
            break;
        default:
            break;
    }
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandDESFireSetLoggingMode(char *OutParam, const char *InParams) {
    if (!IsDESFireConfiguration()) {
        ExitOnInvalidConfigurationError(OutParam);
    }
    char valueStr[16];
    if (!sscanf_P(InParams, PSTR("%15s"), valueStr)) {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
    valueStr[15] = '\0';
    if (!strcasecmp_P(valueStr, PSTR("1")) || !strcasecmp_P(valueStr, PSTR("TRUE")) ||
            !strcasecmp_P(valueStr, PSTR("ON"))) {
        LocalLoggingMode = NORMAL;
        return COMMAND_INFO_OK_ID;
    } else if (!strcasecmp_P(valueStr, PSTR("0")) || !strcasecmp_P(valueStr, PSTR("FALSE")) ||
               !strcasecmp_P(valueStr, PSTR("OFF"))) {
        LocalLoggingMode = OFF;
        return COMMAND_INFO_OK_ID;
    } else if (!strcasecmp_P(valueStr, PSTR("VERBOSE"))) {
        LocalLoggingMode = VERBOSE;
        return COMMAND_INFO_OK_ID;
    } else if (!strcasecmp_P(valueStr, PSTR("DEBUGGING"))) {
        LocalLoggingMode = DEBUGGING;
        return COMMAND_INFO_OK_ID;
    } else {
        snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%s <ON|OFF|VERBOSE|DEBUGGING>"),
                   DFCOMMAND_LOGGING_MODE);
        return COMMAND_ERR_INVALID_USAGE_ID;
    }
}

CommandStatusIdType CommandDESFireGetTestingMode(char *OutParam) {
    if (!IsDESFireConfiguration()) {
        ExitOnInvalidConfigurationError(OutParam);
    } else if (LocalTestingMode) {
        return COMMAND_INFO_TRUE_ID;
    }
    return COMMAND_INFO_FALSE_ID;
}

CommandStatusIdType CommandDESFireSetTestingMode(char *OutParam, const char *InParams) {
    if (!IsDESFireConfiguration()) {
        ExitOnInvalidConfigurationError(OutParam);
    }
    char valueStr[16];
    if (!sscanf_P(InParams, PSTR("%15s"), valueStr)) {
        return COMMAND_ERR_INVALID_PARAM_ID;
    }
    valueStr[15] = '\0';
    if (!strcasecmp_P(valueStr, PSTR("1")) || !strcasecmp_P(valueStr, PSTR("TRUE")) ||
            !strcasecmp_P(valueStr, PSTR("ON"))) {
        LocalTestingMode = 0x01;
        return COMMAND_INFO_TRUE_ID;
    } else if (!strcasecmp_P(valueStr, PSTR("0")) || !strcasecmp_P(valueStr, PSTR("FALSE")) ||
               !strcasecmp_P(valueStr, PSTR("OFF"))) {
        LocalTestingMode = 0x00;
        return COMMAND_INFO_FALSE_ID;
    }
    return COMMAND_ERR_INVALID_USAGE_ID;
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
