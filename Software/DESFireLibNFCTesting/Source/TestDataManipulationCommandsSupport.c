/* TestDataManipulationCommandsSupport.c */

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
        fprintf(stdout, "    -- !! Error selecting new AID by default !!\n");
        return EXIT_FAILURE;
    }    
    else if(Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_AES128,
                         MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating with AES !!\n");
        return EXIT_FAILURE;
    }

    uint8_t aidToCreate[] = { 0x01, 0x00, 0x34 };
    if(CreateApplication(nfcPnd, aidToCreate, 0x0f, 0x03)) {
        fprintf(stdout, "    -- !! Error creating new AID !!\n");
        return EXIT_FAILURE;
    }   
    else if(SelectApplication(nfcPnd, aidToCreate, APPLICATION_AID_LENGTH)) {
        fprintf(stdout, "    -- !! Error selecting new AID by default !!\n");
        return EXIT_FAILURE;
    }   
    else if(Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_AES128,
                         MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating with AES !!\n");
        return EXIT_FAILURE;
    }   

    uint8_t srcDataBuffer[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
    };
    if(CreateStandardDataFile(nfcPnd, 0x00, 0x00, 0x0f, 4)) {
        fprintf(stdout, "    -- !! Error creating standard data file !!\n");
        return EXIT_FAILURE;
    }   
    else if(CreateBackupDataFile(nfcPnd, 0x01, 0x00, 0x0f, 8)) {
        fprintf(stdout, "    -- !! Error creating backup data file !!\n");
        return EXIT_FAILURE;
    }   
    else if(GetFileIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing the file IDs !!\n");
        return EXIT_FAILURE;
    }    
    else if(ReadDataCommand(nfcPnd, 0x00, 0, 4)) {
        fprintf(stdout, "    -- !! Error reading initial contents of file !!\n");
        return EXIT_FAILURE;
    }
    else if(DeleteFile(nfcPnd, 0x01)) {
        fprintf(stdout, "    -- !! Error deleting newly created file !!\n");
        return EXIT_FAILURE;
    }   
    else if(GetFileIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing the file IDs !!\n");
        return EXIT_FAILURE;
    }
    else if(ReadDataCommand(nfcPnd, 0x00, 0, 4)) {
        fprintf(stdout, "    -- !! Error reading initial contents of file !!\n");
        return EXIT_FAILURE;
    }
    else if(ReadDataCommand(nfcPnd, 0x00, 2, 2)) {
        fprintf(stdout, "    -- !! Error reading initial contents of file !!\n");
        return EXIT_FAILURE;
    }
    else if(WriteDataCommand(nfcPnd, 0x00, 0, 4, srcDataBuffer)) {
        fprintf(stdout, "    -- !! Error write data command !!\n");
        return EXIT_FAILURE;
    }
    else if(ReadDataCommand(nfcPnd, 0x00, 0, 4)) {
        fprintf(stdout, "    -- !! Error reading initial contents of file !!\n");
        return EXIT_FAILURE;
    }
    else if(WriteDataCommand(nfcPnd, 0x00, 2, 2, srcDataBuffer + 12)) {
        fprintf(stdout, "    -- !! Error write data command !!\n");
        return EXIT_FAILURE;
    }
    else if(ReadDataCommand(nfcPnd, 0x00, 0, 4)) {
        fprintf(stdout, "    -- !! Error reading initial contents of file !!\n");
        return EXIT_FAILURE;
    }

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
