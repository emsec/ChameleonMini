/* CryptoAES128.c : Adds support for XMega HW accelerated 128-bit AES encryption
 *                  in ECB and CBC modes.
 *                  Based in part on the the source code for Microchip's ASF library
 *                  available at https://github.com/avrxml/asf (see license below).
 * Author: Maxie D. Schmidt (@maxieds)
 */

/*****************************************************************************
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include <avr/interrupt.h>

#include "CryptoAES128.h"

#define NOP()	__asm__ __volatile__("nop")

/* AES interrupt callback function pointer. */
static aes_callback_t __CryptoAESCallbackFunc = NULL;

/* Keep track of the last IV block data */
static CryptoAESBlock_t __CryptoAES_IVData = { 0 };

/* Set the last operation mode (ECB or CBC) init for the context */
uint8_t __CryptoAESOpMode = CRYPTO_AES_ECB_MODE;

void aes_start(void) {
    AES.CTRL |= AES_START_bm;
}

void aes_software_reset(void) {
    AES.CTRL = AES_RESET_bm;
}

bool aes_is_busy(void) {
    return !(AES.STATUS & (AES_SRIF_bm | AES_ERROR_bm));
}

bool aes_is_error(void) {
    return (AES.STATUS & AES_ERROR_bm);
}

void aes_clear_interrupt_flag(void) {
    AES.STATUS |= AES_SRIF_bm;
}

void aes_clear_error_flag(void) {
    AES.STATUS |= AES_ERROR_bm;
}

void aes_configure(CryptoAESDec_t op_mode, CryptoAESAuto_t auto_start, CryptoAESXor_t xor_mode) {
    AES.CTRL = ((uint8_t) op_mode | (uint8_t) auto_start | (uint8_t) xor_mode);
}

void aes_configure_encrypt(CryptoAESAuto_t auto_start, CryptoAESXor_t xor_mode) {
    aes_configure(AES_ENCRYPT, auto_start, xor_mode);
}

void aes_configure_decrypt(CryptoAESAuto_t auto_start, CryptoAESXor_t xor_mode) {
    aes_configure(AES_DECRYPT, auto_start, xor_mode);
}

void aes_set_key(uint8_t *key_in) {
    uint8_t i;
    uint8_t *temp_key = key_in;
    for (i = 0; i < CRYPTO_AES_KEY_SIZE; i++) {
        AES.KEY = *(temp_key++);
    }
}

void aes_get_key(uint8_t *key_out) {
    uint8_t i;
    uint8_t *temp_key = key_out;
    for (i = 0; i < CRYPTO_AES_KEY_SIZE; i++) {
        *(temp_key++) = AES.KEY;
    }
}

static void CryptoAESEncryptBlock(uint8_t *Plaintext, uint8_t *Ciphertext, const uint8_t *Key, bool XorModeOn);
static void CryptoAESDecryptBlock(uint8_t *Plaintext, uint8_t *Ciphertext, const uint8_t *Key);

static bool aes_lastsubkey_generate(uint8_t *key, uint8_t *last_sub_key) {
    bool keygen_ok;
    aes_software_reset();
    /* Load dummy data into AES state memory. It isn't important what is
     * written, just that a write cycle occurs. */
    uint8_t dummy_data[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    CryptoAESEncryptBlock(dummy_data, dummy_data, key, false);
    /* If not error. */
    if (!aes_is_error()) {
        /* Store the last subkey. */
        aes_get_key(last_sub_key);
        aes_clear_interrupt_flag();
        keygen_ok = true;
    } else {
        aes_clear_error_flag();
        keygen_ok = false;
    }
    return keygen_ok;
}

void aes_write_inputdata(uint8_t *data_in) {
    uint8_t i;
    uint8_t *temp_state = data_in;
    for (i = 0; i < CRYPTO_AES_BLOCK_SIZE; i++) {
        AES.STATE = *(temp_state++);
    }
}

void aes_read_outputdata(uint8_t *data_out) {
    uint8_t i;
    uint8_t *temp_state = data_out;
    for (i = 0; i < CRYPTO_AES_BLOCK_SIZE; i++) {
        *(temp_state++) = AES.STATE;
    }
}

void aes_isr_configure(CryptoAESIntlvl_t intlvl) {
    /* Remove pending AES interrupts. */
    AES.STATUS = (AES_ERROR_bm | AES_SRIF_bm);
    AES.INTCTRL = intlvl;
}

void aes_set_callback(const aes_callback_t callback) {
    __CryptoAESCallbackFunc = callback;
}

ISR(AES_INT_vect) {
    if (__CryptoAESCallbackFunc != NULL) {
        __CryptoAESCallbackFunc();
    }
}

void CryptoAESGetConfigDefaults(CryptoAESConfig_t *ctx) {
    if (ctx == NULL) {
        return;
    }
    ctx->ProcessingMode = CRYPTO_AES_PMODE_ENCIPHER;
    ctx->ProcessingDelay = 0;
    ctx->StartMode = AES_MANUAL;
    ctx->XorMode = AES_XOR_ON;
    if (__CryptoAESOpMode == CRYPTO_AES_CBC_MODE) {
        ctx->OpMode = CRYPTO_AES_CBC_MODE;
    } else {
        ctx->OpMode = CRYPTO_AES_ECB_MODE;
    }
}

static void int_callback_aes(void) {}

void CryptoAESInitContext(CryptoAESConfig_t *ctx) {
    if (ctx == NULL) {
        return;
    }
    aes_software_reset();
    memset(__CryptoAES_IVData, 0x00, CRYPTO_AES_BLOCK_SIZE);
    aes_configure(ctx->ProcessingMode, ctx->StartMode, ctx->XorMode);
    aes_set_callback(&int_callback_aes);
}

static uint16_t CryptoAESGetPaddedBufferSize(uint16_t bufSize);
static uint16_t CryptoAESGetPaddedBufferSize(uint16_t bufSize) {
    uint16_t spareBytes = (bufSize % CRYPTO_AES_BLOCK_SIZE);
    if (spareBytes == 0) {
        return bufSize;
    }
    return bufSize + CRYPTO_AES_BLOCK_SIZE - spareBytes;
}

static void CryptoAESEncryptBlock(uint8_t *Plaintext, uint8_t *Ciphertext, const uint8_t *Key, bool XorModeOn) {
    aes_software_reset();
    AES.CTRL = AES_RESET_bm;
    NOP();
    AES.CTRL = 0;
    aes_configure_encrypt(AES_MANUAL, XorModeOn ? AES_XOR_ON : AES_XOR_OFF);
    aes_isr_configure(AES_INTLVL_OFF);
    aes_set_key(Key);
    for (uint8_t i = 0; i < CRYPTO_AES_BLOCK_SIZE; i++) {
        AES.STATE = 0x00;
    }
    aes_write_inputdata(Plaintext);
    aes_start();
    do {
        /* Wait until AES is finished or an error occurs. */
    } while (aes_is_busy());
    aes_read_outputdata(Ciphertext);
    aes_clear_interrupt_flag();
}

static void CryptoAESDecryptBlock(uint8_t *Plaintext, uint8_t *Ciphertext, const uint8_t *Key) {
    AES.CTRL = AES_RESET_bm;
    NOP();
    AES.CTRL = 0;
    uint8_t lastSubKey[CRYPTO_AES_KEY_SIZE];
    aes_lastsubkey_generate(Key, lastSubKey);
    AES.CTRL = AES_RESET_bm;
    NOP();
    AES.CTRL = 0;
    aes_configure_decrypt(AES_MANUAL, AES_XOR_OFF);
    aes_isr_configure(AES_INTLVL_OFF);
    aes_set_key(lastSubKey);
    for (uint8_t i = 0; i < CRYPTO_AES_BLOCK_SIZE; i++) {
        AES.STATE = 0x00;
    }
    aes_write_inputdata(Ciphertext);
    aes_start();
    do {
        /* Wait until AES is finished or an error occurs. */
    } while (aes_is_busy());
    aes_read_outputdata(Plaintext);
    aes_clear_interrupt_flag();
}

int CryptoAESEncryptBuffer(uint16_t Count, uint8_t *Plaintext, uint8_t *Ciphertext,
                           uint8_t *IVIn, const uint8_t *Key) {
    uint8_t *IV = IVIn;
    if ((Count % CRYPTO_AES_BLOCK_SIZE) != 0) {
        return 0xBE;
    }
    if (IVIn == NULL) {
        memset(__CryptoAES_IVData, 0x00, CRYPTO_AES_BLOCK_SIZE);
        IV = &__CryptoAES_IVData[0];
    }
    CryptoAESBlock_t inputBlock;
    size_t bufBlocks = (Count + CRYPTO_AES_BLOCK_SIZE - 1) / CRYPTO_AES_BLOCK_SIZE;
    bool unevenBlockSize = (Count % CRYPTO_AES_BLOCK_SIZE) != 0;
    for (int blk = 0; blk < bufBlocks; blk++) {
        if (__CryptoAESOpMode == CRYPTO_AES_CBC_MODE) {
            if (blk + 1 != bufBlocks || !unevenBlockSize) {
                if (blk == 0) {
                    memcpy(inputBlock, &Plaintext[0], CRYPTO_AES_BLOCK_SIZE);
                    CryptoMemoryXOR(IV, inputBlock, CRYPTO_AES_BLOCK_SIZE);
                } else {
                    memcpy(inputBlock, &Ciphertext[(blk - 1) * CRYPTO_AES_BLOCK_SIZE], CRYPTO_AES_BLOCK_SIZE);
                    CryptoMemoryXOR(&Plaintext[blk * CRYPTO_AES_BLOCK_SIZE], inputBlock, CRYPTO_AES_BLOCK_SIZE);
                }
            } else {
                uint8_t numInputUnevenBytes = Count % CRYPTO_AES_BLOCK_SIZE;
                memcpy(inputBlock, &Plaintext[blk * CRYPTO_AES_BLOCK_SIZE], numInputUnevenBytes);
                memset(&inputBlock[numInputUnevenBytes], 0x00, CRYPTO_AES_BLOCK_SIZE - numInputUnevenBytes);
                if (blk == 0) {
                    CryptoMemoryXOR(IV, inputBlock, CRYPTO_AES_BLOCK_SIZE);
                } else {
                    CryptoMemoryXOR(&Ciphertext[(blk - 1) * CRYPTO_AES_BLOCK_SIZE], inputBlock, CRYPTO_AES_BLOCK_SIZE);
                }
            }
            CryptoAESEncryptBlock(inputBlock, Ciphertext + blk * CRYPTO_AES_BLOCK_SIZE, Key, true);
            if (blk + 1 == bufBlocks) {
                memcpy(IV, inputBlock, CRYPTO_AES_BLOCK_SIZE);
            }
        } else {
            if (blk + 1 != bufBlocks || !unevenBlockSize) {
                memcpy(inputBlock, Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
            } else {
                uint8_t numInputUnevenBytes = Count % CRYPTO_AES_BLOCK_SIZE;
                memcpy(inputBlock, &Plaintext[blk * CRYPTO_AES_BLOCK_SIZE], numInputUnevenBytes);
                memset(&inputBlock[numInputUnevenBytes], 0x00, CRYPTO_AES_BLOCK_SIZE - numInputUnevenBytes);
            }
            CryptoMemoryXOR(IV, inputBlock, CRYPTO_AES_BLOCK_SIZE);
            CryptoAESEncryptBlock(inputBlock, Ciphertext + blk * CRYPTO_AES_BLOCK_SIZE, Key, true);
            memcpy(IV, Ciphertext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
        }
    }
    if (aes_is_error()) {
        aes_clear_error_flag();
        return AES.STATUS & AES_ERROR_bm;
    } else if (unevenBlockSize) {
        return CRYPTO_AES_EXIT_UNEVEN_BLOCKS;
    } else {
        return CRYPTO_AES_EXIT_SUCCESS;
    }
}

int CryptoAESDecryptBuffer(uint16_t Count, uint8_t *Plaintext, uint8_t *Ciphertext,
                           uint8_t *IVIn, const uint8_t *Key) {
    uint8_t *IV = IVIn;
    if (IVIn == NULL) {
        memset(__CryptoAES_IVData, 0x00, CRYPTO_AES_BLOCK_SIZE);
        IV = &__CryptoAES_IVData[0];
    }
    CryptoAESBlock_t inputBlock;
    size_t bufBlocks = (Count + CRYPTO_AES_BLOCK_SIZE - 1) / CRYPTO_AES_BLOCK_SIZE;
    bool unevenBlockSize = (Count % CRYPTO_AES_BLOCK_SIZE) != 0;
    for (int blk = 0; blk < bufBlocks; blk++) {
        if (__CryptoAESOpMode == CRYPTO_AES_CBC_MODE) {
            if (blk + 1 != bufBlocks || !unevenBlockSize) {
                CryptoAESDecryptBlock(inputBlock, Ciphertext + blk * CRYPTO_AES_BLOCK_SIZE, Key);
                if (blk == 0) {
                    memcpy(Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, inputBlock, CRYPTO_AES_BLOCK_SIZE);
                    CryptoMemoryXOR(IV, Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
                } else {
                    memcpy(Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, inputBlock, CRYPTO_AES_BLOCK_SIZE);
                    CryptoMemoryXOR(&Ciphertext[(blk - 1) * CRYPTO_AES_BLOCK_SIZE],
                                    Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
                }
            } else {
                uint8_t numInputUnevenBytes = Count % CRYPTO_AES_BLOCK_SIZE;
                CryptoAESBlock_t inputBlockTemp;
                memset(inputBlockTemp, 0x00, CRYPTO_AES_BLOCK_SIZE);
                memcpy(inputBlockTemp, &Ciphertext[blk * CRYPTO_AES_BLOCK_SIZE], numInputUnevenBytes);
                CryptoAESDecryptBlock(inputBlock, inputBlockTemp, Key);
                if (blk == 0) {
                    memcpy(Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, inputBlock, CRYPTO_AES_BLOCK_SIZE);
                    CryptoMemoryXOR(IV, Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
                } else {
                    memcpy(Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, inputBlock, CRYPTO_AES_BLOCK_SIZE);
                    CryptoMemoryXOR(&Ciphertext[(blk - 1) * CRYPTO_AES_BLOCK_SIZE],
                                    Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
                }
            }
            if (blk + 1 == bufBlocks) {
                memcpy(IV, inputBlock, CRYPTO_AES_BLOCK_SIZE);
            }
        } else {
            if (blk + 1 != bufBlocks || !unevenBlockSize) {
                CryptoAESDecryptBlock(Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, Ciphertext + blk * CRYPTO_AES_BLOCK_SIZE, Key);
                CryptoMemoryXOR(IV, Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
                memcpy(IV, Ciphertext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
            } else {
                uint8_t numInputUnevenBytes = Count % CRYPTO_AES_BLOCK_SIZE;
                memset(inputBlock, 0x00, CRYPTO_AES_BLOCK_SIZE);
                memcpy(inputBlock, Ciphertext + blk * CRYPTO_AES_BLOCK_SIZE, numInputUnevenBytes);
                CryptoAESDecryptBlock(Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, inputBlock, Key);
                CryptoMemoryXOR(IV, Plaintext + blk * CRYPTO_AES_BLOCK_SIZE, CRYPTO_AES_BLOCK_SIZE);
                memcpy(IV, inputBlock, CRYPTO_AES_BLOCK_SIZE);
            }
        }
    }
    if (aes_is_error()) {
        aes_clear_error_flag();
        return AES.STATUS & AES_ERROR_bm;
    } else if (unevenBlockSize) {
        return CRYPTO_AES_EXIT_UNEVEN_BLOCKS;
    } else {
        return CRYPTO_AES_EXIT_SUCCESS;
    }
}

// This routine performs the CBC "send" mode chaining: C = E(P ^ IV); IV = C
static void CryptoAES_CBCSend(uint16_t Count, void *Plaintext, void *Ciphertext, uint8_t *IV, uint8_t *Key, CryptoAES_CBCSpec_t CryptoSpec);
static void CryptoAES_CBCSend(uint16_t Count, void *Plaintext, void *Ciphertext,
                              uint8_t *IV, uint8_t *Key,
                              CryptoAES_CBCSpec_t CryptoSpec) {
    uint16_t numBlocks = CRYPTO_BYTES_TO_BLOCKS(Count, CryptoSpec.blockSize);
    uint16_t blockIndex = 0;
    uint8_t *ptBuf = (uint8_t *) Plaintext, *ctBuf = (uint8_t *) Ciphertext;
    uint8_t tempBlock[CryptoSpec.blockSize], ivBlock[CryptoSpec.blockSize];
    bool lastBlockPadding = false;
    if (numBlocks * CryptoSpec.blockSize > Count) {
        lastBlockPadding = true;
    }
    while (blockIndex < numBlocks) {
        if (blockIndex + 1 == numBlocks && lastBlockPadding) {
            return;
        }
        memcpy(tempBlock, ptBuf + blockIndex * CryptoSpec.blockSize, CryptoSpec.blockSize);
        memcpy(ivBlock, IV, CryptoSpec.blockSize);
        CryptoMemoryXOR(ivBlock, tempBlock, CryptoSpec.blockSize);
        CryptoSpec.cryptFunc(ivBlock, tempBlock, Key);
        memcpy(IV + blockIndex * CryptoSpec.blockSize, tempBlock, CryptoSpec.blockSize);
        memcpy(ctBuf + blockIndex * CryptoSpec.blockSize, tempBlock, CryptoSpec.blockSize);
        blockIndex++;
    }
}

// This routine performs the CBC "receive" mode chaining: C = E(P) ^ IV; IV = P
static void CryptoAES_CBCRecv(uint16_t Count, void *Plaintext, void *Ciphertext, uint8_t *IV, uint8_t *Key, CryptoAES_CBCSpec_t CryptoSpec);
static void CryptoAES_CBCRecv(uint16_t Count, void *Plaintext, void *Ciphertext,
                              uint8_t *IV, uint8_t *Key,
                              CryptoAES_CBCSpec_t CryptoSpec) {
    uint16_t numBlocks = CRYPTO_BYTES_TO_BLOCKS(Count, CryptoSpec.blockSize);
    uint16_t blockIndex = 0;
    uint8_t *ptBuf = (uint8_t *) Plaintext, *ctBuf = (uint8_t *) Ciphertext;
    uint8_t tempBlock[CryptoSpec.blockSize], ivBlock[CryptoSpec.blockSize];
    bool lastBlockPadding = false;
    if (numBlocks * CryptoSpec.blockSize > Count) {
        lastBlockPadding = true;
    }
    while (blockIndex < numBlocks) {
        if (blockIndex + 1 == numBlocks && lastBlockPadding) {
            return;
        }
        memcpy(ivBlock, ptBuf + blockIndex * CryptoSpec.blockSize, CryptoSpec.blockSize);
        CryptoSpec.cryptFunc(ivBlock, tempBlock, Key);
        memcpy(ivBlock, IV, CryptoSpec.blockSize);
        CryptoMemoryXOR(ivBlock, tempBlock, CryptoSpec.blockSize);
        memcpy(IV, ptBuf + blockIndex * CryptoSpec.blockSize, CryptoSpec.blockSize);
        memcpy(ctBuf + blockIndex * CryptoSpec.blockSize, ivBlock, CryptoSpec.blockSize);
        blockIndex++;
    }
}

#ifdef ENABLE_CRYPTO_TESTS
void CryptoAESDecrypt_CBCSend(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                              uint8_t *Key, uint8_t *IV) {
    CryptoAES_CBCSpec_t CryptoSpec = {
        .cryptFunc   = &CryptoAESDecryptBlock,
        .blockSize   = CRYPTO_AES_BLOCK_SIZE
    };
    CryptoAES_CBCSend(Count, PlainText, CipherText, IV, Key, CryptoSpec);
}

void CryptoAESDecrypt_CBCReceive(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                                 uint8_t *Key, uint8_t *IV) {
    CryptoAES_CBCSpec_t CryptoSpec = {
        .cryptFunc   = &CryptoAESDecryptBlock,
        .blockSize   = CRYPTO_AES_BLOCK_SIZE
    };
    CryptoAES_CBCRecv(Count, PlainText, CipherText, IV, Key, CryptoSpec);
}
#endif

void CryptoAESEncrypt_CBCSend(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                              uint8_t *Key, uint8_t *IV) {
    CryptoAES_CBCSpec_t CryptoSpec = {
        .cryptFunc   = &CryptoAESEncryptBlock,
        .blockSize   = CRYPTO_AES_BLOCK_SIZE
    };
    CryptoAES_CBCSend(Count, PlainText, CipherText, IV, Key, CryptoSpec);
}

void CryptoAESEncrypt_CBCReceive(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                                 uint8_t *Key, uint8_t *IV) {
    CryptoAES_CBCSpec_t CryptoSpec = {
        .cryptFunc   = &CryptoAESEncryptBlock,
        .blockSize   = CRYPTO_AES_BLOCK_SIZE
    };
    CryptoAES_CBCRecv(Count, PlainText, CipherText, IV, Key, CryptoSpec);
}

uint16_t appendBufferCRC32C(uint8_t *bufferData, uint16_t bufferSize) {
    uint32_t workingCRC = INIT_CRC32C_VALUE;
    for (int i = 0; i < bufferSize; i++) {
        workingCRC = workingCRC ^ *(bufferData++);
        for (int j = 0; j < 8; j++) {
            if (workingCRC & 1) {
                workingCRC = (workingCRC >> 1) ^ LE_CRC32C_POLYNOMIAL;
            } else {
                workingCRC = workingCRC >> 1;
            }
        }
    }
    // Append the CRC32C bytes in little endian byte order to the end of the buffer:
    bufferData[bufferSize] = (uint8_t)(workingCRC & 0x000000FF);
    bufferData[bufferSize + 1] = (uint8_t)((workingCRC & 0x0000FF00) >> 8);
    bufferData[bufferSize + 2] = (uint8_t)((workingCRC & 0x00FF0000) >> 16);
    bufferData[bufferSize + 4] = (uint8_t)((workingCRC & 0xFF000000) >> 24);
    return bufferSize + 4;
}
