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
#include "DESFireUtils.h"
#include "DESFireISO7816Support.h"

#include "../ISO14443-3A.h"

/* General structure of a ISO 14443-4 block:
 * PCB (protocol control byte)
 * CID (card identifier byte; presence controlled by PCB)
 * NAD (node address byte; presence controlled by PCB)
 * Payload (arbitrary bytes)
 * CRC-16
 */

/* Refer to Table 10 in section 9.3 (page 15) of the NXP Mifare Classic EV1 1K data sheet:
 * https://www/nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf
 */
#define ISO14443A_ACK                       0xA
#define ISO14443A_NAK                       0x0
#define ISO14443A_NAK_PARITY_ERROR          0x1

/* See Table 13 in section 7.1 (page 67) of the NXP PN532 User Manual (error Handling / status codes):
 * https://www.nxp.com/docs/en/user-guide/141520.pdf
 */
#define ISO14443A_CRCA_ERROR                0x02
#define ISO14443A_CRC_FRAME_SIZE            ASBITS(ISO14443A_CRCA_SIZE)

#define ISO14443A_CMD_RATS                  0xE0
#define ISO14443A_RATS_FRAME_SIZE           ASBITS(6)

#define NXP_PN532_CMD_INDESELECT            0x44
#define IsDeselectCmd(cmdCode)              (cmdCode == NXP_PN532_CMD_INDESELECT)
#define ISO14443A_DESELECT_FRAME_SIZE       (ISO14443A_HLTA_FRAME_SIZE + ASBITS(ISO14443A_CRCA_SIZE))

#define NXP_PN532_INSELECT_CMD              0x54
#define ISO14443ACmdIsPM3WUPA(cmdCode)      (cmdCode == NXP_PN532_INSELECT_CMD)
#define ISO14443ACmdIsWUPA(cmdCode)         ((cmdCode == ISO14443A_CMD_WUPA) || ISO14443ACmdIsPM3WUPA(cmdCode))

#define IsRIDCmd(cmdCode)                   (cmdCode == ISO7816_RID_CMD)

/* A quick way to catch and handle the bytes the Hid Omnikey 5022CL and ACR-122 USB readers
 * will throw out to identify NFC tags in a promiscuous blanket enumeration of possibilities
 * while running 'pcsc_spy -v'. The strategy is to respond with a NAK and continue to ignore
 * these incoming commands.
 */
#define MIFARE_RESTORE_CMD                  0xC2
#define NFCTAG_TYPE12_TERMINATOR_TLV        0xFE
#define NXP_PN532_CMD_TGGETINITIATOR        0x88
#define IsUnsupportedCmd(cmdCode)           ((cmdCode == MIFARE_RESTORE_CMD) || (cmdCode == NFCTAG_TYPE12_TERMINATOR_TLV) || (cmdCode == NXP_PN532_CMD_TGGETINITIATOR))

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

#define ISO14443_R_BLOCK_SIZE               1

#define ISO14443_PCB_S_DESELECT             (ISO14443_PCB_S_BLOCK_STATIC)
#define ISO14443_PCB_S_DESELECT_V2          0xCA
#define ISO14443_PCB_S_WTX                  (ISO14443_PCB_S_BLOCK_STATIC | 0x30)
#define ISO14443A_CMD_PPS                   0xD0

#define IS_ISO14443A_4_COMPLIANT(bufByte)   (bufByte & 0x20)
#define MAKE_ISO14443A_4_COMPLIANT(bufByte) (bufByte |= 0x20)

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

/* Configure saving last data frame state so can resend on ACK from the PCD */

#define MAX_DATA_FRAME_XFER_SIZE            (72)
extern uint8_t  ISO14443ALastIncomingDataFrame[MAX_DATA_FRAME_XFER_SIZE];
extern uint16_t ISO14443ALastIncomingDataFrameBits;
uint16_t ISO14443AStoreLastDataFrameAndReturn(const uint8_t *Buffer, uint16_t BufferBitCount);

#define MAX_STATE_RETRY_COUNT               (0x18)
extern uint8_t StateRetryCount;
bool CheckStateRetryCount(bool resetByDefault);
bool CheckStateRetryCountWithLogging(bool resetByDefault, bool performLogging);

/* Support functions */
void ISO144434SwitchState(Iso144434StateType NewState);
void ISO144434SwitchStateWithLogging(Iso144434StateType NewState, bool performLogging);

void ISO144434Reset(void);
uint16_t ISO144434ProcessBlock(uint8_t *Buffer, uint16_t ByteCount, uint16_t BitCount);

/*
 * ISO/IEC 14443-3A implementation
 */
#define ISO14443A_CRCA_INIT                 ((uint16_t) 0x6363)
uint16_t ISO14443AUpdateCRCA(const uint8_t *Buffer, uint16_t ByteCount, uint16_t InitCRCA);

#define GetAndSetBufferCRCA(Buffer, ByteCount)     ({                                \
     uint16_t fullReturnBits = 0;                                                    \
     ISO14443AAppendCRCA(Buffer, ByteCount);                                         \
     fullReturnBits = ASBITS(ByteCount) + ISO14443A_CRC_FRAME_SIZE;                  \
     fullReturnBits;                                                                 \
     })

#define GetAndSetNoResponseCRCA(Buffer)            ({                                \
     uint16_t fullReturnBits = 0;                                                    \
     ISO14443AUpdateCRCA(Buffer, 0, ISO14443A_CRCA_INIT);                            \
     fullReturnBits = ISO14443A_CRC_FRAME_SIZE;                                      \
     fullReturnBits;                                                                 \
     })

typedef enum DESFIRE_FIRMWARE_ENUM_PACKING {
    /* The card is powered up but not selected: */
    ISO14443_3A_STATE_IDLE = ISO14443_4_STATE_LAST + 1,
    /* Entered on REQA or WUP --  anticollision is being performed: */
    ISO14443_3A_STATE_READY_CL1,
    ISO14443_3A_STATE_READY_CL1_NVB_END,
    ISO14443_3A_STATE_READY_CL2,
    ISO14443_3A_STATE_READY_CL2_NVB_END,
    /* Entered when the card has been selected: */
    ISO14443_3A_STATE_ACTIVE,
    /* Something went wrong or we've received a halt command: */
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
