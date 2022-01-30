/* TestAuthenticateAES128.c */

#include "LibNFCUtils.h"
#include "LibNFCWrapper.h"
#include "DesfireUtils.h"
#include "CryptoUtils.h"

int main(int argc, char **argv) {

    if(!Test3DESEncyptionRoutines()) {
        return EXIT_FAILURE;
    }

    nfc_context *nfcCtxt;
    nfc_device  *nfcPnd = GetNFCDeviceDriver(&nfcCtxt);
    if(nfcPnd == NULL) {
         return EXIT_FAILURE;
    }

    // Select AID application 0x000000:
    if(SelectApplication(nfcPnd, MASTER_APPLICATION_AID, APPLICATION_AID_LENGTH)) {
        return EXIT_FAILURE;
    }

    // Get list of application IDs:
    if(GetApplicationIds(nfcPnd)) {
        return EXIT_FAILURE;
    }
    // Start ISO authentication (default key, blank setting of all zeros):
    if(Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_ISODES, 
                    MASTER_KEY_INDEX, ZERO_KEY)) {
        return EXIT_FAILURE;
    }                  

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}

