/* TestAuthenticateAES256.c 
 * NOTE: Need to run the following Chameleon terminal command before running 
 *       this test case: ``DF_COMM_MODE=Enciphered:AES256``
 */

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

    // Start AES authentication (default key, blank setting of all zeros):
    if (Authenticate(nfcPnd, DESFIRE_CRYPTO_AUTHTYPE_AES256,
                     MASTER_KEY_INDEX, ZERO_KEY)) {
        return EXIT_FAILURE;
    }

    FreeNFCDeviceDriver(&nfcCtxt, &nfcPnd);
    return EXIT_SUCCESS;

}

