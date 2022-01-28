/*
The DESFire stack portion of this firmware source
is free software written by Maxie Dion Schmidt (@maxieds):
You can redistribute it and/or modify
it under the terms of this license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

The complete source distribution of
this firmware is available at the following link:
https://github.com/maxieds/ChameleonMiniFirmwareDESFireStack.

Based in part on the original DESFire code created by
@dev-zzo (GitHub handle) [Dmitry Janushkevich] available at
https://github.com/dev-zzo/ChameleonMini/tree/desfire.

This notice must be retained at the top of all source files where indicated.
*/

/*
 * DESFireISO14443Support.h
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifndef __DESFIRE_ISO14443_SUPPORT_H__
#define __DESFIRE_ISO14443_SUPPORT_H__

#include "DESFireFirmwareSettings.h"

#include "../ISO14443-3A.h"
#include "../../Codec/ISO14443-2A.h"

/* General structure of a ISO 14443-4 block:
 * PCB (protocol control byte)
 * CID (card identifier byte; presence controlled by PCB)
 * NAD (node address byte; presence controlled by PCB)
 * Payload (arbitrary bytes)
 * CRC-16
 */

#define ISO14443A_CMD_RATS                  0xE0
#define ISO14443A_RATS_FRAME_SIZE           (6 * BITS_PER_BYTE) //(4 * 8) /* Bit */
#define ISO14443A_CMD_RNAK                  0xB2
#define ISO14443A_CRC_FRAME_SIZE           (ISO14443A_CRCA_SIZE * BITS_PER_BYTE)

#define ISO14443_PCB_BLOCK_TYPE_MASK        0xC0
#define ISO14443_PCB_I_BLOCK                0x00
#define ISO14443_PCB_R_BLOCK                0x80
#define ISO14443_PCB_S_BLOCK                0xC0

#define ISO14443_PCB_I_BLOCK_STATIC         0x02
#define ISO14443_PCB_R_BLOCK_STATIC         0xA2
#define ISO14443_PCB_S_BLOCK_STATIC         0xC2

#define ISO14443_PCB_BLOCK_NUMBER_MASK      0x01
#define ISO14443_PCB_HAS_NAD_MASK           0x04
#define ISO14443_PCB_HAS_CID_MASK           0x08
#define ISO14443_PCB_I_BLOCK_CHAINING_MASK  0x10
#define ISO14443_PCB_R_BLOCK_ACKNAK_MASK    0x10
#define ISO14443_PCB_R_BLOCK_ACK            0x00
#define ISO14443_PCB_R_BLOCK_NAK            0x10

#define ISO14443_R_BLOCK_SIZE               1 /* Bytes */

#define ISO14443_PCB_S_DESELECT             (ISO14443_PCB_S_BLOCK_STATIC)
#define ISO14443_PCB_S_DESELECT_V2          0xCA
#define ISO14443_PCB_S_WTX                  (ISO14443_PCB_S_BLOCK_STATIC | 0x30)
#define ISO14443A_CMD_PPS                   0xD0

#define IS_ISO14443A_4_COMPLIANT(buf)       (buf[0] & 0x20)
#define MAKE_ISO14443A_4_COMPLIANT(buf)     (buf[0] |= 0x20)

/*
 * ISO/IEC 14443-4 implementation
 * To support EV2 cards emulation, proper support for handling 14443-4
 * blocks will be implemented.
 * Currently NOT supported:
 * + Block chaining
 * + Frame waiting time extension
 */

typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
    ISO14443_4_STATE_EXPECT_RATS,
    ISO14443_4_STATE_ACTIVE,
    ISO14443_4_STATE_LAST,
} Iso144434StateType;

extern Iso144434StateType Iso144434State;
extern uint8_t Iso144434BlockNumber;
extern uint8_t Iso144434CardID;
extern uint8_t LastReaderSentCmd;

/* Setup some fuzzy response handling for problematic readers like the ACR122U */
#define MAX_STATE_RETRY_COUNT               (4)
extern uint8_t StateRetryCount;
bool CheckStateRetryCount(bool resetByDefault);
bool CheckStateRetryCount2(bool resetByDefault, bool performLogging);

#define IGNORE_ACK_BYTE               (0x92)

/* Support functions */
void ISO144434SwitchState(Iso144434StateType NewState);
void ISO144434SwitchState2(Iso144434StateType NewState, bool performLogging);

void ISO144434Reset(void);
static uint16_t ISO144434ProcessBlock(uint8_t *Buffer, uint16_t ByteCount, uint16_t BitCount);

/*
 * ISO/IEC 14443-3A implementation
 */
#define ISO14443A_CRCA_INIT      ((uint16_t) 0x6363)

#define GetAndSetBufferCRCA(Buffer, ByteCount)     ({                                \
     uint16_t fullReturnBits = 0;                                                    \
     ISO14443AAppendCRCA(Buffer, ByteCount);                                         \
     fullReturnBits = ByteCount * BITS_PER_BYTE + ISO14443A_CRC_FRAME_SIZE;          \
     fullReturnBits;                                                                 \
     })
/* TODO: Should we return CRC bytes when giving a NO_RESPONSE ??? ... Apparently "No". */
#define GetAndSetNoResponseCRCA(Buffer)            ({                                \
     uint16_t fullReturnBits = 0;                                                    \
     ISO14443AAppendCRCA(Buffer, 0);                                                 \
     fullReturnBits = ISO14443A_CRC_FRAME_SIZE;                                      \
     fullReturnBits;                                                                 \
     })

uint16_t ISO14443AUpdateCRCA(const uint8_t *Buffer, uint16_t ByteCount, uint16_t InitCRCA);

typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
    /* The card is powered up but not selected */
    ISO14443_3A_STATE_IDLE = ISO14443_4_STATE_LAST,
    /* Entered on REQA or WUPA; anticollision is being performed */
    ISO14443_3A_STATE_READY1,
    ISO14443_3A_STATE_READY2,
    /* Entered when the card has been selected */
    ISO14443_3A_STATE_ACTIVE,
    /* Something went wrong or we've received a halt command */
    ISO14443_3A_STATE_HALT,
} Iso144433AStateType;

extern Iso144433AStateType Iso144433AState;
extern Iso144433AStateType Iso144433AIdleState;

/* Support functions */
void ISO144433ASwitchState(Iso144433AStateType NewState);
void ISO144433AReset(void);
void ISO144433AHalt(void);
bool ISO144433AIsHalt(const uint8_t *Buffer, uint16_t BitCount);
uint16_t ISO144433APiccProcess(uint8_t *Buffer, uint16_t BitCount);

#endif
