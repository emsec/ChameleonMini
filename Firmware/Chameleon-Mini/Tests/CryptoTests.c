/* CryptoTests.c */

#ifdef ENABLE_CRYPTO_TESTS

#include "CryptoTests.h"

bool CryptoTDEATestCase1(char *OutParam, uint16_t MaxOutputLength) {
    const uint8_t ZeroBlock[CRYPTO_DES_BLOCK_SIZE] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    const uint8_t TestKey2KTDEA[CRYPTO_2KTDEA_KEY_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0xcc, 0xaa, 0xff, 0xee, 0x11, 0x33, 0x33, 0x77,
    };
    const uint8_t TestOutput2KTDEAECB[CRYPTO_DES_BLOCK_SIZE] = {
        0xcb, 0x5c, 0xfc, 0xeb, 0x00, 0x25, 0x1b, 0x60,
    };
    uint8_t Output[CRYPTO_DES_BLOCK_SIZE];
    CryptoEncrypt2KTDEA(ZeroBlock, Output, TestKey2KTDEA);
    if (memcmp(TestOutput2KTDEAECB, Output, sizeof(TestOutput2KTDEAECB))) {
        strcat_P(OutParam, PSTR("> "));
        OutParam += 2;
        BufferToHexString(OutParam, MaxOutputLength - 2, Output, CRYPTO_DES_BLOCK_SIZE);
        strcat_P(OutParam, PSTR("\r\n"));
        return false;
    }
    return true;
}

bool CryptoTDEATestCase2(char *OutParam, uint16_t MaxOutputLength) {
    const uint8_t TestKey2KTDEA[CRYPTO_2KTDEA_KEY_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0xcc, 0xaa, 0xff, 0xee, 0x11, 0x33, 0x33, 0x77,
    };
    const uint8_t TestInput2KTDEACBCReceive[CRYPTO_2KTDEA_KEY_SIZE] = {
        0x27, 0xd2, 0x6c, 0x67, 0xc8, 0x49, 0xe5, 0xa5,
        0x42, 0xa6, 0x8f, 0xe6, 0x82, 0x09, 0xa1, 0x1c,
    };
    const uint8_t TestOutput2KTDEACBCReceive[CRYPTO_2KTDEA_KEY_SIZE] = {
        0x13, 0x37, 0xc0, 0xde, 0xca, 0xfe, 0xba, 0xbe,
        0xde, 0xde, 0xde, 0xde, 0xad, 0xad, 0xad, 0xad,
    };
    uint8_t Output[2 * CRYPTO_DES_BLOCK_SIZE];
    uint8_t IV[CRYPTO_DES_BLOCK_SIZE];
    memset(IV, 0x00, sizeof(IV));
    CryptoEncrypt2KTDEA_CBCReceive(2, TestInput2KTDEACBCReceive, Output, IV, TestKey2KTDEA);
    if (memcmp(TestOutput2KTDEACBCReceive, Output, sizeof(TestOutput2KTDEACBCReceive))) {
        strcat_P(OutParam, PSTR("> "));
        OutParam += 2;
        BufferToHexString(OutParam, MaxOutputLength - 2, Output, CRYPTO_2KTDEA_KEY_SIZE);
        strcat_P(OutParam, PSTR("\r\n"));
        return false;
    }
    return true;
}

bool CryptoAESTestCase1(char *OutParam, uint16_t MaxOutputLength) {
    // Key from FIPS-197: 00010203 04050607 08090A0B 0C0D0E0
    const uint8_t KeyData[CRYPTO_AES_KEY_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };
    // Plaintext from FIPS-197: 00112233 44556677 8899AABB CCDDEEFF
    const uint8_t PlainText[CRYPTO_AES_BLOCK_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    // Cipher result from FIPS-197: 69c4e0d8 6a7b0430 d8cdb780 70b4c55a
    const uint8_t CipherText[CRYPTO_AES_BLOCK_SIZE] = {
        0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
        0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a
    };
    uint8_t tempBlock[CRYPTO_AES_BLOCK_SIZE];
    CryptoAESConfig_t aesContext;
    CryptoAESGetConfigDefaults(&aesContext);
    aesContext.OpMode = CRYPTO_AES_CBC_MODE;
    CryptoAESInitContext(&aesContext);
    CryptoAESEncryptBuffer(CRYPTO_AES_BLOCK_SIZE, PlainText, tempBlock, NULL, KeyData);
    if (memcmp(tempBlock, CipherText, CRYPTO_AES_BLOCK_SIZE)) {
        strcat_P(OutParam, PSTR("> ENC: "));
        OutParam += 7;
        BufferToHexString(OutParam, MaxOutputLength - 7, tempBlock, CRYPTO_AES_BLOCK_SIZE);
        strcat_P(OutParam, PSTR("\r\n"));
        return false;
    }
    CryptoAESDecryptBuffer(CRYPTO_AES_BLOCK_SIZE, tempBlock, CipherText, NULL, KeyData);
    if (memcmp(tempBlock, PlainText, CRYPTO_AES_BLOCK_SIZE)) {
        strcat_P(OutParam, PSTR("> DEC: "));
        OutParam += 7;
        BufferToHexString(OutParam, MaxOutputLength - 7, tempBlock, CRYPTO_AES_BLOCK_SIZE);
        strcat_P(OutParam, PSTR("\r\n"));
        return false;
    }
    return true;
}

bool CryptoAESTestCase2(char *OutParam, uint16_t MaxOutputLength) {
    // Example data taken from:
    // https://boringssl.googlesource.com/boringssl/+/2490/crypto/cipher/test/cipher_test.txt#104
    const uint8_t KeyData[CRYPTO_AES_KEY_SIZE] = {
        0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
        0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
    };
    const uint8_t PlainText[CRYPTO_AES_BLOCK_SIZE] = {
        0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96,
        0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A
    };
    const uint8_t CipherText[CRYPTO_AES_BLOCK_SIZE] = {
        0x76, 0x49, 0xAB, 0xAC, 0x81, 0x19, 0xB2, 0x46,
        0xCE, 0xE9, 0x8E, 0x9B, 0x12, 0xE9, 0x19, 0x7D
    };
    const uint8_t IV[CRYPTO_AES_BLOCK_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };
    uint8_t tempBlock[CRYPTO_AES_BLOCK_SIZE];
    CryptoAESConfig_t aesContext;
    CryptoAESGetConfigDefaults(&aesContext);
    aesContext.OpMode = CRYPTO_AES_CBC_MODE;
    CryptoAESInitContext(&aesContext);
    CryptoAESEncryptBuffer(CRYPTO_AES_BLOCK_SIZE, PlainText, tempBlock, IV, KeyData);
    if (memcmp(tempBlock, CipherText, CRYPTO_AES_BLOCK_SIZE)) {
        strcat_P(OutParam, PSTR("> ENC: "));
        OutParam += 7;
        BufferToHexString(OutParam, MaxOutputLength - 7, tempBlock, CRYPTO_AES_BLOCK_SIZE);
        strcat_P(OutParam, PSTR("\r\n"));
        return false;
    }
    CryptoAESDecryptBuffer(CRYPTO_AES_BLOCK_SIZE, tempBlock, CipherText, IV, KeyData);
    if (memcmp(tempBlock, PlainText, CRYPTO_AES_BLOCK_SIZE)) {
        strcat_P(OutParam, PSTR("> DEC: "));
        OutParam += 7;
        BufferToHexString(OutParam, MaxOutputLength - 7, tempBlock, CRYPTO_AES_BLOCK_SIZE);
        strcat_P(OutParam, PSTR("\r\n"));
        return false;
    }
    return true;
}

#endif /* ENABLE_CRYPTO_TESTS */
