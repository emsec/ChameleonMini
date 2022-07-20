/* TestDataManipulationCommandsSupport3.c */

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

    uint8_t aidToCreate[] = { 0xf4, 0xa5, 0x34 };
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

    uint8_t dataBuf[16];
    memset(dataBuf, 0x00, 16);
    uint8_t dataBufLarge[84];
    memset(dataBufLarge, 0x00, 84);

    if (CreateLinearRecordFile(nfcPnd, 0x01, 0x00, 0x0f, 2, 6)) {
        fprintf(stdout, "    -- !! Error creating linear record file !!\n");
        return EXIT_FAILURE;
    } else if (CreateCyclicRecordFile(nfcPnd, 0x02, 0x00, 0x0f, 3, 84)) {
        fprintf(stdout, "    -- !! Error creating cyclic record file !!\n");
        return EXIT_FAILURE;
    } else if (GetFileIds(nfcPnd)) {
        fprintf(stdout, "    -- !! Error listing file IDs !!\n");
        return EXIT_FAILURE;
    } else if (ReadRecordsCommand(nfcPnd, 0x01, 0, 6)) {
        fprintf(stdout, "    -- !! Error reading records !!\n");
        return EXIT_FAILURE;
    } else if (WriteRecordsCommand(nfcPnd, 0x01, 0, 3, dataBuf)) {
        fprintf(stdout, "    -- !! Error writing records !!\n");
        return EXIT_FAILURE;
    } else if (ReadRecordsCommand(nfcPnd, 0x01, 0, 6)) {
        fprintf(stdout, "    -- !! Error reading records !!\n");
        return EXIT_FAILURE;
    } else if (WriteRecordsCommand(nfcPnd, 0x02, 0, 84, dataBufLarge)) {
        fprintf(stdout, "    -- !! Error writing large records !!\n");
        return EXIT_FAILURE;
    } else if (ReadRecordsCommand(nfcPnd, 0x02, 0, 84)) {
        fprintf(stdout, "    -- !! Error reading large records !!\n");
        return EXIT_FAILURE;
    }
    /* TODO: Still need to test the ClearRecordFile command */

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}
