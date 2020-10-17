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

    uint8_t aidToCreateList1[] = {
        0x77, 0x88, 0x99 
    };
    uint8_t aidToCreateList2[] = { 
        0x01, 0x00, 0x34
    };
    if(GetApplicationIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing existing AIDs !!\n");
        return EXIT_FAILURE;
    }
    else if(CreateApplication(nfcPnd, aidToCreateList1, 0x0f, 3) || GetApplicationIds(nfcPnd) || 
            CreateApplication(nfcPnd, aidToCreateList2, 0x0f, 3) || GetApplicationIds(nfcPnd) || 
            CreateApplication(nfcPnd, aidToCreateList1, 0x0f, 3) || GetApplicationIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error creating new AID !!\n");
        return EXIT_FAILURE;
    }
    else if(SelectApplication(nfcPnd, aidToCreateList1, APPLICATION_AID_LENGTH)) {
        fprintf(stdout, "    -- !! Error selecting newly created AID !!\n");
        return EXIT_FAILURE;
    }
    else if(Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_AES128,
                         MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating with AES!!\n");
        return EXIT_FAILURE;
    }
    else if(DeleteApplication(nfcPnd, aidToCreateList1)) {
        fprintf(stdout, "    -- !! Error deleting newly created AID !!\n");
        return EXIT_FAILURE;
    }
    else if(GetApplicationIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing existing AIDs !!\n");
        return EXIT_FAILURE;
    }
    else if(SelectApplication(nfcPnd, MASTER_APPLICATION_AID, APPLICATION_AID_LENGTH)) {
        fprintf(stdout, "    -- !! Error selecting PICC app !!\n");
        return EXIT_FAILURE;
    }
    else if(Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_AES128,
                         MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating with AES!!\n");
        return EXIT_FAILURE;
    }
    else if(DeleteApplication(nfcPnd, aidToCreateList2)) {
        fprintf(stdout, "    -- !! Error deleting second new AID !!\n");
        return EXIT_FAILURE;
    }
    else if(GetApplicationIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing existing AIDs !!\n");
        return EXIT_FAILURE;
    }
    else if(DeleteApplication(nfcPnd, MASTER_APPLICATION_AID)) {
        fprintf(stdout, "    -- !! Error trying to verify no success at deleting PICC app !!\n");
    }
    else if(GetApplicationIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing existing AIDs !!\n");
        return EXIT_FAILURE;
    }
    else if(GetDFNamesCommand(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing DF names !!\n");
        return EXIT_FAILURE;
    }

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
