/* CryptoAES128.h : Adds support for XMega HW accelerated 128-bit AES encryption
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

#ifndef __CRYPTO_AES128_HW_H__
#define __CRYPTO_AES128_HW_H__

#include <avr/io.h>

/* AES Processing mode */
#define CRYPTO_AES_PMODE_DECIPHER          0     // decipher
#define CRYPTO_AES_PMODE_ENCIPHER          1     // cipher

/* AES Start mode */
#define CRYPTO_AES_START_MODE_MANUAL       0     // Manual mode
#define CRYPTO_AES_START_MODE_AUTO         1     // Auto mode
#define CRYPTO_AES_START_MODE_DMA          2     // DMA mode

/* AES Cryptographic key size */
#define CRYPTO_AES_KEY_SIZE_128            0     // 128-bit
#define CRYPTO_AES_KEY_SIZE_192            1     // 192-bit
#define CRYPTO_AES_KEY_SIZE_256            2     // 256-bit

/* AES Operation cipher modes */
#define CRYPTO_AES_ECB_MODE                0     // Electronic Code Book mode
#define CRYPTO_AES_CBC_MODE                1     // Cipher Block Chaining mode
#define CRYPTO_AES_OFB_MODE                2     // Output FeedBack mode (NOT SUPPORTED)
#define CRYPTO_AES_CFB_MODE                3     // Cipher FeedBack mode (NOT SUPPORTED)
#define CRYPTO_AES_CTR_MODE                4     // Counter mode (NOT SUPPORTED)

extern uint8_t __CryptoAESOpMode;

/* AES URAD Type */
#define CRYPTO_AES_URAT_INPUTWRITE_DMA     0     // Input Data Register written during the data processing in DMA mode
#define CRYPTO_AES_URAT_OUTPUTREAD_PROCESS 1     // Output Data Register read during the data processing
#define CRYPTO_AES_URAT_MRWRITE_PROCESS    2     // Mode Register written during the data processing
#define CRYPTO_AES_URAT_OUTPUTREAD_SUBKEY  3     // Output Data Register read during the sub-keys generation
#define CRYPTO_AES_URAT_MRWRITE_SUBKEY     4     // Mode Register written during the sub-keys generation
#define CRYPTO_AES_URAT_READ_WRITEONLY     5     // Write-only register read access

/* Error and status codes */
#define CRYPTO_AES_EXIT_SUCCESS            0
#define CRYPTO_AES_EXIT_UNEVEN_BLOCKS      1

/* Define types for the AES-128 specific implementation: */
#define CRYPTO_AES_KEY_SIZE                16
typedef uint8_t CryptoAESKey_t[CRYPTO_AES_KEY_SIZE];

#define CRYPTO_AES_BLOCK_SIZE	           16
typedef uint8_t CryptoAESBlock_t[CRYPTO_AES_BLOCK_SIZE];

#define CryptoAESBytesToBlocks(byteCount)  ((byteCount + CRYPTO_AES_BLOCK_SIZE - 1) / CRYPTO_AES_BLOCK_SIZE)

typedef enum aes_dec {
    AES_ENCRYPT,
    AES_DECRYPT = AES_DECRYPT_bm,
} CryptoAESDec_t;

/* AES Auto Start Trigger settings */
typedef enum aes_auto {
    AES_MANUAL,                      // manual start mode.
    AES_AUTO = AES_AUTO_bm,          // auto start mode.
} CryptoAESAuto_t;

/* AES State XOR Load Enable settings */
typedef enum aes_xor {
    AES_XOR_OFF,                    // state direct load.
    AES_XOR_ON = AES_XOR_bm,        // state XOR load.
} CryptoAESXor_t;

/* AES Interrupt Enable / Level settings */
typedef enum aes_intlvl {
    AES_INTLVL_OFF = AES_INTLVL_OFF_gc,  // Interrupt Disabled
    AES_INTLVL_LO  = AES_INTLVL_LO_gc,   // Low Level
    AES_INTLVL_MED = AES_INTLVL_MED_gc,  // Medium Level
    AES_INTLVL_HI  = AES_INTLVL_HI_gc,   // High Level
} CryptoAESIntlvl_t;

/* AES interrupt callback function pointer. */
typedef void (*aes_callback_t)(void);

#define CRYPTO_AES128_STRUCT_ATTR               __attribute((aligned(1)))

typedef struct {
    CryptoAESDec_t   ProcessingMode;
    uint8_t          ProcessingDelay;            // [0,15]
    CryptoAESAuto_t  StartMode;
    unsigned char    OpMode;                     // 0 = ECB, 1 = CBC, 2 = OFB, 3 = CFB, 4 = CTR
    CryptoAESXor_t   XorMode;
} CryptoAESConfig_t CRYPTO_AES128_STRUCT_ATTR;

typedef struct {
    unsigned char   datrdy;                      // ENABLE/DISABLE; Data ready interrupt
    unsigned char   urad;                        // ENABLE/DISABLE; Unspecified Register Access Detection
} CryptoAES_ISRConfig_t CRYPTO_AES128_STRUCT_ATTR;

/* AES encryption complete. */
#define AES_ENCRYPTION_COMPLETE  (1UL << 0)

/* AES GF multiplication complete. */
#define AES_GF_MULTI_COMPLETE    (1UL << 1)

void aes_start(void);
void aes_software_reset(void);
bool aes_is_busy(void);
bool aes_is_error(void);
void aes_clear_interrupt_flag(void);
void aes_clear_error_flag(void);
void aes_configure(CryptoAESDec_t op_mode, CryptoAESAuto_t auto_start, CryptoAESXor_t xor_mode);
void aes_configure_encrypt(CryptoAESAuto_t auto_start, CryptoAESXor_t xor_mode);
void aes_configure_decrypt(CryptoAESAuto_t auto_start, CryptoAESXor_t xor_mode);
void aes_set_key(uint8_t *key_in);
void aes_get_key(uint8_t *key_out);
void aes_write_inputdata(uint8_t *data_in);
void aes_read_outputdata(uint8_t *data_out);
void aes_isr_configure(CryptoAESIntlvl_t intlvl);
void aes_set_callback(const aes_callback_t callback);

void CryptoAESGetConfigDefaults(CryptoAESConfig_t *ctx);
void CryptoAESInitContext(CryptoAESConfig_t *ctx);

int CryptoAESEncryptBuffer(uint16_t Count, uint8_t *Plaintext, uint8_t *Ciphertext,
                           uint8_t *IV, const uint8_t *Key);
int CryptoAESDecryptBuffer(uint16_t Count, uint8_t *Plaintext, uint8_t *Ciphertext,
                           uint8_t *IV, const uint8_t *Key);

typedef uint8_t (*CryptoAESFuncType)(uint8_t *, uint8_t *, uint8_t *);
typedef struct {
    CryptoAESFuncType  cryptFunc;
    uint16_t           blockSize;
} CryptoAES_CBCSpec_t CRYPTO_AES128_STRUCT_ATTR;

#ifdef ENABLE_CRYPTO_TESTS
void CryptoAESDecrypt_CBCSend(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                              uint8_t *Key, uint8_t *IV);
void CryptoAESDecrypt_CBCReceive(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                                 uint8_t *Key, uint8_t *IV);
#endif

void CryptoAESEncrypt_CBCReceive(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                                 uint8_t *Key, uint8_t *IV);
void CryptoAESEncrypt_CBCSend(uint16_t Count, uint8_t *PlainText, uint8_t *CipherText,
                              uint8_t *Key, uint8_t *IV);

/* Crypto utility functions: */
#define CRYPTO_BYTES_TO_BLOCKS(numBytes, blockSize)    ( ((numBytes) + (blockSize) - 1) / (blockSize) )

#define CryptoMemoryXOR(inputBuf, destBuf, bufSize) ({ \
     uint8_t *in = (uint8_t *) inputBuf;               \
     uint8_t *out = (uint8_t *) destBuf;               \
     uint16_t count = (uint16_t) bufSize;              \
     while(count-- > 0) {                              \
          *(out++) ^= *(in++);                         \
     }                                                 \
     })

/* The initial value is (-1) in one's complement: */
#define INIT_CRC32C_VALUE            ((uint32_t) 0xFFFFFFFF)

/* The CCITT CRC-32 polynomial (modulo 2) in little endian (lsb-first) byte order: */
#define LE_CRC32C_POLYNOMIAL         ((uint32_t) 0xEDB88320)

uint16_t appendBufferCRC32C(uint8_t *bufferData, uint16_t bufferSize);

#endif
