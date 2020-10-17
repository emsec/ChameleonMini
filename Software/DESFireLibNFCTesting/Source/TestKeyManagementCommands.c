/* TestKeyManagmentCommands.c */

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

    if(ChangeKeyCommand(nfcPnd, 0x00, ZERO_KEY, DESFIRE_CRYPTO_AUTHTYPE_AES128)) {
        fprintf(stdout, "    -- !! GetVersion failed !!\n");
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
    
    if(GetKeySettingsCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! GetKeySettings failed !!\n");
        return EXIT_FAILURE;
    }
    else if(ChangeKeySettingsCommand(nfcPnd, 0x0f)) {
        fprintf(stdout, "    -- !! ChangeKeySettings failed !!\n");
        return EXIT_FAILURE;
    }
    else if(GetKeyVersionCommand(nfcPnd, 0x00)) {
        fprintf(stdout, "    -- !! GetKeyVersion failed !!\n");
        return EXIT_FAILURE;
    }

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
