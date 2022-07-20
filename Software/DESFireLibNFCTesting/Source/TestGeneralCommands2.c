/* TestGeneralCommands.c */

#include <stdlib.h>
#include <stdio.h>

#include <nfc/nfc.h>

#include "LibNFCUtils.h"
#include "LibNFCWrapper.h"
#include "DesfireUtils.h"


int main(int argc, char **argv) {

    nfc_context *nfcCtxt;
    nfc_device  *nfcPnd = GetNFCDeviceDriver(&nfcCtxt);
    if (nfcPnd == NULL) {
        return EXIT_FAILURE;
    }

    if (Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_LEGACY, MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating !!\n");
        return EXIT_FAILURE;
    } else if (GetCardUIDCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! GetCardUID failed !!\n");
        return EXIT_FAILURE;
    } else if (FreeMemoryCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! FreeMemory failed !!\n");
        return EXIT_FAILURE;
    }
    /* NOTE: Not tested: SetConfigurationCommand. */
    /* NOTE: Not tested: FormattPiccCommand. */

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
