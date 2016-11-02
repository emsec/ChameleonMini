#include "CryptoTDEA.h"

#include "../Terminal/Terminal.h"
#include <stdarg.h>

static void DebugPrintP(const char *fmt, ...)
{
    char Format[80];
    char Buffer[80];
    va_list args;

    strcpy_P(Format, fmt);
    va_start(args, fmt);
    vsnprintf(Buffer, sizeof(Buffer), Format, args);
    va_end(args);
    TerminalSendString(Buffer);
}

static const uint8_t ZeroBlock[CRYPTO_DES_BLOCK_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t TestKey2KTDEA[CRYPTO_2KTDEA_KEY_SIZE] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0xcc, 0xaa, 0xff, 0xee, 0x11, 0x33, 0x33, 0x77,
};
static const uint8_t TestOutput2KTDEAECB[CRYPTO_DES_BLOCK_SIZE] = {
    0xcb, 0x5c, 0xfc, 0xeb, 0x00, 0x25, 0x1b, 0x60,
};

static const uint8_t TestInput2KTDEACBCReceive[CRYPTO_2KTDEA_KEY_SIZE] = {
    0x27, 0xd2, 0x6c, 0x67, 0xc8, 0x49, 0xe5, 0xa5,
    0x42, 0xa6, 0x8f, 0xe6, 0x82, 0x09, 0xa1, 0x1c,
};
static const uint8_t TestOutput2KTDEACBCReceive[CRYPTO_2KTDEA_KEY_SIZE] = {
    0x13, 0x37, 0xc0, 0xde, 0xca, 0xfe, 0xba, 0xbe,
    0xde, 0xde, 0xde, 0xde, 0xad, 0xad, 0xad, 0xad,
};

void RunTDEATests(void)
{
    uint8_t Output[CRYPTO_DES_BLOCK_SIZE * 4];
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];

    DebugPrintP(PSTR("\r\nTDEA BIST starting"));

    /* Test 2KTDEA ecnryption, ECB mode */
    CryptoEncrypt2KTDEA(ZeroBlock, Output, TestKey2KTDEA);
    if (memcmp(TestOutput2KTDEAECB, Output, sizeof(TestOutput2KTDEAECB))) {
        DebugPrintP(PSTR("\r\n2KTDEA ecnryption, ECB mode FAILED"));
        DebugPrintP(PSTR("\r\nOut = %02X%02X%02X%02X%02X%02X%02X%02X"),
            Output[0],Output[1],Output[2],Output[3],
            Output[4],Output[5],Output[6],Output[7]);
    }

    /* Test 2KTDEA encryption, CBC receive mode */
    memset(IV, 0, sizeof(IV));
    CryptoEncrypt2KTDEA_CBCReceive(2, TestInput2KTDEACBCReceive, Output, IV, TestKey2KTDEA);
    if (memcmp(TestOutput2KTDEACBCReceive, Output, sizeof(TestOutput2KTDEACBCReceive))) {
        DebugPrintP(PSTR("\r\n2KTDEA encryption, CBC receive mode FAILED"));
        DebugPrintP(PSTR("\r\nOut = %02X%02X%02X%02X%02X%02X%02X%02X"),
            Output[0],Output[1],Output[2],Output[3],
            Output[4],Output[5],Output[6],Output[7]);
    }

    /* TODO: test other modes */

    DebugPrintP(PSTR("\r\nTDEA BIST completed\r\n"));
}
