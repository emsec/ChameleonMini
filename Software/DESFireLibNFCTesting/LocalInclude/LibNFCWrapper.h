/* LibNFCWrapper.h */

#ifndef __LIBNFC_WRAPPER_H__
#define __LIBNFC_WRAPPER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>

#include <nfc/nfc.h>

#include "LibNFCUtils.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

static inline nfc_device *GetNFCDeviceDriver(nfc_context **context) {
    nfc_init(context);
    if (*context == NULL) {
        ERR("Unable to init libnfc (malloc)");
        return NULL;
    }
    nfc_device *pnd = nfc_open(*context, NULL);
    if (pnd == NULL) {
        ERR("Error opening NFC reader");
        nfc_exit(context);
        *context = NULL;
        return NULL;
    }
    if (nfc_initiator_init(pnd) < 0) {
        nfc_perror(pnd, "nfc_initiator_init");
        nfc_close(pnd);
        nfc_exit(context);
        *context = NULL;
        return NULL;
    }
    // Configure some convenient common settings:
    nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, false);
    nfc_device_set_property_bool(pnd, NP_HANDLE_CRC, true);
    nfc_device_set_property_bool(pnd, NP_HANDLE_PARITY, true);
    nfc_device_set_property_bool(pnd, NP_AUTO_ISO14443_4, true);
    nfc_device_set_property_bool(pnd, NP_ACTIVATE_FIELD, true);
    nfc_device_set_property_bool(pnd, NP_EASY_FRAMING, false);
    nfc_device_set_property_int(pnd, NP_TIMEOUT_COMMAND, 0);
    return pnd;
}

static inline void FreeNFCDeviceDriver(nfc_context **context, nfc_device **device) {
    if (*device != NULL) {
        nfc_close(*device);
    }
    if (*context != NULL) {
        nfc_exit(*context);
    }
}

#endif
