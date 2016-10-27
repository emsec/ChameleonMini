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

static void Test1(void)
{
    static uint8_t TestKey[CRYPTO_2KTDEA_KEY_SIZE] = {
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    static uint8_t TestInput[CRYPTO_DES_BLOCK_SIZE] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    static uint8_t TestOutput[CRYPTO_DES_BLOCK_SIZE] = {
        0xfa, 0xfd, 0x50, 0x84, 0x37, 0x4f, 0xce, 0x34,
    };
    uint8_t RealOutput[CRYPTO_DES_BLOCK_SIZE] = {0};

    CryptoEncrypt_2KTDEA_CBC(1, TestInput, RealOutput, TestKey);
    if (memcmp(TestOutput, RealOutput, CRYPTO_DES_BLOCK_SIZE)) {
        DebugPrintP(PSTR("\r\nTDEA TEST 1 FAILED\r\n"));
        DebugPrintP(PSTR("Out = %02X%02X%02X%02X%02X%02X%02X%02X\r\n"),
            RealOutput[0],RealOutput[1],RealOutput[2],RealOutput[3],
            RealOutput[4],RealOutput[5],RealOutput[6],RealOutput[7]);
    }
}

void RunTDEATests(void)
{
    Test1();
}
