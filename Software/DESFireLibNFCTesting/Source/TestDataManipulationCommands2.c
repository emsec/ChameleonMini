/* TestDataManipulationCommandsSupport2.c */

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

    if (Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_ISODES, MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating !!\n");
        return EXIT_FAILURE;
    }

    uint8_t aidToCreate[] = { 0xf3, 0x12, 0x34 };
    if (CreateApplication(nfcPnd, aidToCreate, 0x0f, 1)) {
        fprintf(stdout, "    -- !! Error creating new AID !!\n");
        return EXIT_FAILURE;
    } else if (SelectApplication(nfcPnd, aidToCreate, APPLICATION_AID_LENGTH)) {
        fprintf(stdout, "    -- !! Error selecting new AID by default !!\n");
        return EXIT_FAILURE;
    } else if (Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_ISODES, MASTER_KEY_INDEX, ZERO_KEY)) {
        fprintf(stdout, "    -- !! Error authenticating !!\n");
        return EXIT_FAILURE;
    }

    if (CreateValueFile(nfcPnd, 0x02, 0x00, 0x0f, 0, 256, 128, 0x01)) {
        fprintf(stdout, "    -- !! Error creating value file !!\n");
        return EXIT_FAILURE;
    } else if (GetFileIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing the file IDs !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x02)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (CreditValueFileCommand(nfcPnd, 0x02, 64)) {
        fprintf(stdout, "    -- !! Error crediting value file !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x02)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (DebitValueFileCommand(nfcPnd, 0x02, 64)) {
        fprintf(stdout, "    -- !! Error debiting value file !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x02)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (LimitedCreditValueFileCommand(nfcPnd, 0x02, 32)) {
        fprintf(stdout, "    -- !! Error applying limited credit !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x02)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (CommitTransaction(nfcPnd)) {
        fprintf(stdout, "    -- !! Error committing transaction !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x02)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (DebitValueFileCommand(nfcPnd, 0x02, 200) ||
               CommitTransaction(nfcPnd) ||
               GetValueCommand(nfcPnd, 0x02)) {
        fprintf(stdout, "    -- !! Error trying to remove more than available !!\n");
        return EXIT_FAILURE;
    }

    if (CreateValueFile(nfcPnd, 0x03, 0x00, 0x0f, 0, 256, 128, 0x00)) {
        fprintf(stdout, "    -- !! Error creating value file !!\n");
        return EXIT_FAILURE;
    } else if (GetFileIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing the file IDs !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x03)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (CreditValueFileCommand(nfcPnd, 0x03, 64)) {
        fprintf(stdout, "    -- !! Error crediting value file !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x03)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (DebitValueFileCommand(nfcPnd, 0x03, 64)) {
        fprintf(stdout, "    -- !! Error debiting value file !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x03)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (LimitedCreditValueFileCommand(nfcPnd, 0x03, 32)) {
        fprintf(stdout, "    -- !! Error applying limited credit !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x03)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    } else if (AbortTransaction(nfcPnd)) {
        fprintf(stdout, "    -- !! Error committing transaction !!\n");
        return EXIT_FAILURE;
    } else if (GetValueCommand(nfcPnd, 0x03)) {
        fprintf(stdout, "    -- !! Error fetching value from file !!\n");
        return EXIT_FAILURE;
    }

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
