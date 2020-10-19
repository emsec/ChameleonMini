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
    if(nfcPnd == NULL) {
         return EXIT_FAILURE;
    }   

    if(SelectApplication(nfcPnd, MASTER_APPLICATION_AID, APPLICATION_AID_LENGTH)) {
        fprintf(stdout, "    -- !! Error selecting PICC (Master) AID by default !!\n");
        return EXIT_FAILURE;
    }   
    else if(Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_AES128,
                         MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating with AES !!\n");
        return EXIT_FAILURE;
    }

    if(GetVersionCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! GetVersion failed !!\n");
        return EXIT_FAILURE;
    }   
    else if(FormatPiccCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! FormatPICC failed !!\n");
        return EXIT_FAILURE;
    }
    else if(GetCardUIDCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! GetCardUID failed !!\n");
        return EXIT_FAILURE;
    }
    else if(SetConfigurationCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! SetConfiguration failed !!\n");
        return EXIT_FAILURE;
    }
    else if(FreeMemoryCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! FreeMemory failed !!\n");
        return EXIT_FAILURE;
    }

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
