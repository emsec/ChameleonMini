/* Iso7816Utils.h */

#ifndef __ISO7816_UTILS_H__
#define __ISO7816_UTILS_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <nfc/nfc.h>

#include "ErrorHandling.h"
#include "Config.h"
#include "CryptoUtils.h"
#include "LibNFCUtils.h"
#include "GeneralUtils.h"

static inline int Iso7816SelectCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

static inline int Iso7816GetChallengeCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

static inline int Iso7816ExternalAuthenticateCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

static inline int Iso7816InternalAuthenticateCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

static inline int Iso7816ReadBinaryCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

static inline int Iso7816UpdateBinaryCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

static inline int Iso7816ReadRecordsCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

static inline int Iso7816AppendRecordCommand(nfc_device *nfcConnDev) {
    if(nfcConnDev == NULL) {
        return INVALID_PARAMS_ERROR;
    }
    return EXIT_SUCCESS;
}

#endif
