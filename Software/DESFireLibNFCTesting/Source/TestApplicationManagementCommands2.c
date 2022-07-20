/* TestApplicationManagementCommands.c */

#include <stdlib.h>
#include <stdio.h>

#include <nfc/nfc.h>

#include "LibNFCUtils.h"
#include "LibNFCWrapper.h"
#include "DesfireUtils.h"
#include "CryptoUtils.h"

int main(int argc, char **argv) {

    nfc_context *nfcCtxt;
    nfc_device  *nfcPnd = GetNFCDeviceDriver(&nfcCtxt);
    if (nfcPnd == NULL) {
        return EXIT_FAILURE;
    }

    uint8_t aidToCreateList1[] = {
        0x77, 0x88, 0x99
    };

    if (Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_LEGACY, MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating !!\n");
        return EXIT_FAILURE;
    } else if (CreateApplication(nfcPnd, aidToCreateList1, 0x0f, 2)) {
        fprintf(stdout, "    -- !! Error creating new AID !!\n");
        return EXIT_FAILURE;
    } else if (GetApplicationIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing existing AIDs !!\n");
        return EXIT_FAILURE;
    } else if (DeleteApplication(nfcPnd, aidToCreateList1)) {
        fprintf(stdout, "    -- !! Error deleting newly created AID !!\n");
        return EXIT_FAILURE;
    } else if (GetApplicationIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing existing AIDs !!\n");
        return EXIT_FAILURE;
    }

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
