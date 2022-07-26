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
 * DESFireISO14443Support.c
 * Maxie D. Schmidt (github.com/maxieds)
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "../Application.h"
#include "../Reader14443A.h"
#include "../MifareDESFire.h"

#include "DESFireISO14443Support.h"
#include "DESFirePICCControl.h"
#include "DESFireLogging.h"

/*
 * ISO/IEC 14443-4 implementation
 */
Iso144434StateType Iso144434State = ISO14443_4_STATE_EXPECT_RATS;

uint8_t Iso144434BlockNumber = 0;
uint8_t Iso144434CardID = 1;
uint8_t Iso144434LastBlockLength = 0;
uint8_t StateRetryCount = 0;

uint8_t  ISO14443ALastIncomingDataFrame[MAX_DATA_FRAME_XFER_SIZE] = { 0x00 };
uint16_t ISO14443ALastIncomingDataFrameBits = 0;

bool CheckStateRetryCountWithLogging(bool resetByDefault, bool performLogging) {
    if (resetByDefault || ++StateRetryCount > MAX_STATE_RETRY_COUNT) {
        ISO144434SwitchStateWithLogging(Iso144433AIdleState, performLogging);
        StateRetryCount = 0;
        return true;
    }
    return false;
}
bool CheckStateRetryCount(bool resetByDefault) {
    return CheckStateRetryCountWithLogging(resetByDefault, true);
}

void ISO144434SwitchStateWithLogging(Iso144434StateType NewState, bool performLogging) {
    Iso144434State = NewState;
    StateRetryCount = 0;
    if (DesfireDebuggingOn && performLogging) {
        DesfireLogISOStateChange(Iso144434State, LOG_ISO14443_4_STATE);
    }
}

void ISO144434SwitchState(Iso144434StateType NewState) {
    ISO144434SwitchStateWithLogging(NewState, true);
}

void ISO144434Reset(void) {
    /* No logging here -- spams the log and slows things way down! */
    ISO144433AReset();
    Iso144434State = ISO14443_4_STATE_EXPECT_RATS;
    Iso144434BlockNumber = 1;
    Iso144434CardID = 1;
    ISO14443ALastIncomingDataFrameBits = 0;
    ISO14443ALastIncomingDataFrame[0] = 0x00;
}

static uint16_t GetACKCommandData(uint8_t *Buffer);
static uint16_t GetACKCommandData(uint8_t *Buffer) {
    Buffer[0] = ISO14443A_ACK;
    return 4;
}

static uint16_t GetNAKCommandData(uint8_t *Buffer, bool ResetToHaltState);
static uint16_t GetNAKCommandData(uint8_t *Buffer, bool ResetToHaltState) {
    if (ResetToHaltState) {
        ISO144433AHalt();
        StateRetryCount = 0;
    }
    Buffer[0] = ISO14443A_NAK;
    return 4;
}

static uint16_t GetNAKParityErrorCommandData(uint8_t *Buffer, bool ResetToHaltState);
static uint16_t GetNAKParityErrorCommandData(uint8_t *Buffer, bool ResetToHaltState) {
    if (ResetToHaltState) {
        ISO144433AHalt();
        StateRetryCount = 0;
    }
    Buffer[0] = ISO14443A_NAK_PARITY_ERROR;
    return 4;
}

uint16_t ISO144434ProcessBlock(uint8_t *Buffer, uint16_t ByteCount, uint16_t BitCount) {

    uint8_t PCB = Buffer[0];
    uint8_t MyBlockNumber = Iso144434BlockNumber;
    uint8_t PrologueLength = 0;
    uint8_t HaveCID = 0, HaveNAD = 0;

    /* Verify the block's length: at the very least PCB + CRCA */
    if (ByteCount < 1 + ISO14443A_CRCA_SIZE) {
        /* Broken frame -- Respond with error by returning an empty frame */
        //return ISO14443A_APP_NO_RESPONSE;
    } else {
        ByteCount -= 2;
    }

    /* Verify the checksum; fail if doesn't match */
    if (ByteCount >= 1 + ISO14443A_CRCA_SIZE && !ISO14443ACheckCRCA(Buffer, ByteCount)) {
        DesfireLogEntry(LOG_ERR_APP_CHECKSUM_FAIL, (uint8_t *) NULL, 0);
        /* ISO/IEC 14443-4, clause 7.5.5. The PICC does not attempt any error recovery. */
        DEBUG_PRINT_P(PSTR("ISO14443-4: CRC fail"));
        /* Invalid data received -- Respond with NAK */
        return GetNAKParityErrorCommandData(Buffer, false);
        //return ISO14443A_APP_NO_RESPONSE;
    }

    switch (Iso144434State) {

        case ISO14443_4_STATE_EXPECT_RATS: {
            /* See: ISO/IEC 14443-4, clause 5.6.1.2 */
            if (Buffer[0] != ISO14443A_CMD_RATS) {
                /* Ignore blocks other than RATS and HLTA */
                DEBUG_PRINT_P(PSTR("ISO14443-4: NOT-RATS"));
                return GetNAKCommandData(Buffer, false);
                //return ISO14443A_APP_NO_RESPONSE;
            }
            /* Process RATS:
             * NOTE: ATS bytes are tailored to Chameleon implementation and differ from DESFire spec.
             * NOTE: Some PCD implementations do a memcmp() over ATS bytes, which is completely wrong.
             */
            Iso144434CardID = Buffer[1] & 0x0F;
            Buffer[0] = 0x06;
            memcpy(&Buffer[1], &Picc.ATSBytes[1], 4);
            Buffer[5] = 0x80;                              /* T1: dummy value for historical bytes */
            ByteCount = 6;
            ISO144434SwitchState(ISO14443_4_STATE_ACTIVE);
            return GetAndSetBufferCRCA(Buffer, ByteCount); /* PM3 'hf mfdes list' expects CRCA bytes on the RATS data */
        }
        case ISO14443_4_STATE_ACTIVE: {
            /* See: ISO/IEC 14443-4; 7.1 Block format */

            /* The next case should not happen: it is a baudrate change: */
            if ((Buffer[0] & 0xF0) == ISO14443A_CMD_PPS) {
                ISO144434SwitchState(ISO14443_4_STATE_ACTIVE);
                return GetAndSetBufferCRCA(Buffer, 1);
            }

            /* Parse the prologue */
            PrologueLength = 1;
            HaveCID = PCB & ISO14443_PCB_HAS_CID_MASK;
            if (HaveCID) {
                PrologueLength++;
                /* Verify the card ID */
                if (ByteCount > 1 && (Buffer[1] & 0xF) != Iso144434CardID) {
                    /* Different card ID -- the frame is ignored */
                    DEBUG_PRINT_P(PSTR("ISO14443-4: NEW-CARD-ID %02x != %02x"), (Buffer[1] & 0xF), Iso144434CardID);
                    return GetNAKCommandData(Buffer, false);
                    //return ISO14443A_APP_NO_RESPONSE;
                }
            }
            break;
        }
        case ISO14443_4_STATE_LAST: {
            return GetNAKCommandData(Buffer, false);
            //return ISO14443A_APP_NO_RESPONSE;
        }
        default:
            break;
    }

    switch (PCB & ISO14443_PCB_BLOCK_TYPE_MASK) {

        case ISO14443_PCB_I_BLOCK: {
            HaveNAD = PCB & ISO14443_PCB_HAS_NAD_MASK;
            if (HaveNAD) {
                PrologueLength++;
                /* Not currently supported -- the frame is ignored */
                DEBUG_PRINT_P(PSTR("ISO144434-4: ISO14443_PCB_I_BLOCK"));
            }
            /* 7.5.3.2, rule D: toggle on each I-block */
            Iso144434BlockNumber = MyBlockNumber = !MyBlockNumber;
            if (PCB & ISO14443_PCB_I_BLOCK_CHAINING_MASK) {
                /* Currently not supported -- the frame is ignored */
                DEBUG_PRINT_P(PSTR("ISO14443-4: ISO14443_PCB_I_BLOCK"));
                return GetNAKCommandData(Buffer, false);
                //return ISO14443A_APP_NO_RESPONSE;
            }

            /* Build the prologue for the response */
            /* NOTE: According to the std, incoming/outgoing prologue lengths are equal at all times */
            PCB = ISO14443_PCB_I_BLOCK_STATIC | MyBlockNumber;
            if (HaveCID) {
                PCB |= ISO14443_PCB_HAS_CID_MASK;
                Buffer[1] = Iso144434CardID;
            }
            Buffer[0] = PCB;
            /* Let the DESFire application code process the input data */
            ByteCount = MifareDesfireProcessCommand(&Buffer[PrologueLength], ByteCount - PrologueLength);
            /* Short-circuit in case the app decides not to respond at all */
            if (ByteCount == 0) {
                return GetNAKCommandData(Buffer, false);
                //return ISO14443A_APP_NO_RESPONSE;
            }
            ByteCount += PrologueLength;
            DEBUG_PRINT_P(PSTR("ISO14443-4: I-BLK"));
            return GetAndSetBufferCRCA(Buffer, ByteCount);
        }

        case ISO14443_PCB_R_BLOCK: {
            /* 7.5.4.3, rule 11 */
            if ((PCB & ISO14443_PCB_BLOCK_NUMBER_MASK) == MyBlockNumber) {
                DEBUG_PRINT_P(PSTR("ISO144434-4: ISO14443_PCB_R_BLOCK"));
                return GetNAKCommandData(Buffer, false);
                //return ISO14443A_APP_NO_RESPONSE;
            }
            if (PCB & ISO14443_PCB_R_BLOCK_ACKNAK_MASK) {
                /* 7.5.4.3, rule 12 */
                /* This is a NAK. Send an ACK back */
                Buffer[0] = ISO14443_PCB_R_BLOCK_STATIC | ISO14443_PCB_R_BLOCK_ACK | MyBlockNumber;
                /* The NXP data sheet MF1S50YYX_V1 (Table 10: ACK / NAK) says we should return 4 bits: */
                return 4;
            } else {
                /* This is an ACK: */
                /* NOTE: Chaining is not supported yet. */
                DEBUG_PRINT_P(PSTR("ISO144434-4: ISO14443_PCB_R_BLOCK"));
                // Resend the data from the last frame:
                if (ISO14443ALastIncomingDataFrameBits > 0) {
                    memcpy(&Buffer[0], &ISO14443ALastIncomingDataFrame[0], ASBYTES(ISO14443ALastIncomingDataFrameBits));
                    return ISO14443ALastIncomingDataFrameBits;
                } else {
                    return GetNAKCommandData(Buffer, false);
                    //return ISO14443A_APP_NO_RESPONSE;
                }
            }
            DEBUG_PRINT_P(PSTR("ISO14443-4: R-BLK"));
            return GetAndSetBufferCRCA(Buffer, ByteCount);
        }

        case ISO14443_PCB_S_BLOCK: {
            if ((PCB == ISO14443_PCB_S_DESELECT) || (PCB == ISO14443_PCB_S_DESELECT_V2)) {
                /* Reset our state */
                ISO144434Reset();
                DesfireLogISOStateChange(Iso144434State, LOG_ISO14443_4_STATE);
                /* Transition to HALT */
                ISO144433AHalt();
                /* Answer with S(DESELECT) -- just send the copy of the message */
                ByteCount = PrologueLength;
                DEBUG_PRINT_P(PSTR("ISO14443-4: S-BLK"));
                return GetAndSetBufferCRCA(Buffer, ByteCount);
            }
            DEBUG_PRINT_P(PSTR("ISO14443-4: PCB_S_BLK NAK"));
            return GetNAKCommandData(Buffer, false);
            //return ISO14443A_APP_NO_RESPONSE;
        }

        default:
            break;

    }

    /* Fall through: */
    return GetNAKCommandData(Buffer, false);
    //return ISO14443A_APP_NO_RESPONSE;

}

/*
 * ISO/IEC 14443-3A implementation
 */

#include <util/crc16.h>
uint16_t ISO14443AUpdateCRCA(const uint8_t *Buffer, uint16_t ByteCount, uint16_t InitCRCA) {
    uint16_t Checksum = InitCRCA;
    uint8_t *DataPtr = (uint8_t *) Buffer;
    while (ByteCount--) {
        uint8_t Byte = *DataPtr++;
        Checksum = _crc_ccitt_update(Checksum, Byte);
    }
    DataPtr[1] = (Checksum >> 8) & 0x00FF;
    DataPtr[0] = Checksum & 0x00FF;
    return Checksum;
}

Iso144433AStateType Iso144433AState = ISO14443_3A_STATE_IDLE;
Iso144433AStateType Iso144433AIdleState = ISO14443_3A_STATE_IDLE;

void ISO144433ASwitchState(Iso144433AStateType NewState) {
    Iso144433AState = NewState;
    DesfireLogISOStateChange(Iso144433AState, LOG_ISO14443_3A_STATE);
}

void ISO144433AReset(void) {
    /* No logging performed -- spams the log and slows things way down! */
    Iso144433AState = ISO14443_3A_STATE_IDLE;
    Iso144433AIdleState = ISO14443_3A_STATE_IDLE;
    StateRetryCount = 0;
    ISO14443ALastIncomingDataFrame[0] = 0x00;
    ISO14443ALastIncomingDataFrameBits = 0;
}

void ISO144433AHalt(void) {
    ISO144433AReset();
    ISO144433ASwitchState(ISO14443_3A_STATE_HALT);
}

bool ISO144433AIsHalt(const uint8_t *Buffer, uint16_t BitCount) {
    return BitCount == ISO14443A_HLTA_FRAME_SIZE + ASBITS(ISO14443A_CRCA_SIZE) &&
           Buffer[0] == ISO14443A_CMD_HLTA &&
           Buffer[1] == 0x00 &&
           ISO14443ACheckCRCA(Buffer, ASBYTES(ISO14443A_HLTA_FRAME_SIZE));
}
static uint16_t GetHLTACommandData(uint8_t *Buffer, bool ResetToHaltState);
static uint16_t GetHLTACommandData(uint8_t *Buffer, bool ResetToHaltState) {
    if (ResetToHaltState) {
        ISO144433AHalt();
        StateRetryCount = 0;
    }
    Buffer[0] == ISO14443A_CMD_HLTA;
    Buffer[1] == 0x00;
    ISO14443AAppendCRCA(Buffer, ASBYTES(ISO14443A_HLTA_FRAME_SIZE));
    return ISO14443A_HLTA_FRAME_SIZE + ASBITS(ISO14443A_CRCA_SIZE);
}

uint16_t ISO144433APiccProcess(uint8_t *Buffer, uint16_t BitCount) {

    if (BitCount == 0) {
        return ISO14443A_APP_NO_RESPONSE;
    }

    uint8_t Cmd = Buffer[0];

    /* Wakeup and Request may occure in all states */
    if (Cmd == ISO14443A_CMD_REQA) {
        DesfireLogEntry(LOG_INFO_APP_CMD_REQA, NULL, 0);
        ISO144433ASwitchState(ISO14443_3A_STATE_IDLE);
        StateRetryCount = 0;
    } else if (ISO14443ACmdIsWUPA(Cmd)) {
        DesfireLogEntry(LOG_INFO_APP_CMD_WUPA, NULL, 0);
        ISO144433ASwitchState(ISO14443_3A_STATE_IDLE);
        StateRetryCount = 0;
    } else if (ISO144433AIsHalt(Buffer, BitCount)) {
        DesfireLogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
        return GetHLTACommandData(Buffer, true);
    } else if (Cmd == ISO14443A_CMD_RATS) {
        ISO144434SwitchState(ISO14443_4_STATE_EXPECT_RATS);
        uint16_t ReturnBits = ISO144434ProcessBlock(Buffer, ASBYTES(BitCount), BitCount);
        Iso144433AState = ISO14443_3A_STATE_ACTIVE;
        StateRetryCount = 0;
        return ReturnBits;
    } else if (IsDeselectCmd(Cmd)) {
        ISO144433AHalt();
        return GetACKCommandData(Buffer);
        //return GetHLTACommandData(Buffer, true);
    } else if (IsRIDCmd(Cmd)) {
        Iso144433AState = ISO14443_3A_STATE_ACTIVE;
        StateRetryCount = 0;
        /* Response to RID command as specified in section 7.3.10 (page 139) of the
         * NXP PN532 Manual (InDeselect command specification):
         * https://www.nxp.com/docs/en/user-guide/141520.pdf
         */
        uint16_t respDataSize = (uint16_t) sizeof(MIFARE_DESFIRE_TAG_AID);
        memcpy(&Buffer[0], MIFARE_DESFIRE_TAG_AID, respDataSize);
        return GetAndSetBufferCRCA(Buffer, respDataSize);
    } else if (IsUnsupportedCmd(Cmd)) {
        return GetNAKCommandData(Buffer, true);
    } else if (CheckStateRetryCount(false)) {
        DEBUG_PRINT_P(PSTR("ISO14443-3: SW-RESET"));
        return GetHLTACommandData(Buffer, true);
    } else if (BitCount <= BITS_PER_BYTE) {
        return ISO144434ProcessBlock(Buffer, ASBYTES(BitCount), BitCount);
    }

    /* This implements ISO 14443-3A state machine */
    /* See: ISO/IEC 14443-3, clause 6.2 */
    switch (Iso144433AState) {

        case ISO14443_3A_STATE_HALT:
            if (!ISO14443ACmdIsWUPA(Cmd)) {
                DEBUG_PRINT_P(PSTR("ISO14443-4: HLT-NOT-WUPA"));
                break;
            } else {
                ISO144433ASwitchState(ISO14443_3A_STATE_IDLE);
            }

        case ISO14443_3A_STATE_IDLE:
            Iso144433AIdleState = Iso144433AState;
            ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL1);
            /* The LSByte ordering of the ATQA value for ISO14443 tags is
             * discussed in section 2.3 of NXP AN10833.
             */
            Buffer[0] = Picc.ATQA[1];
            Buffer[1] = Picc.ATQA[0];
            return ASBITS(ISO14443A_ATQA_FRAME_SIZE_BYTES);

        case ISO14443_3A_STATE_READY_CL1:
        case ISO14443_3A_STATE_READY_CL1_NVB_END: {
            if (Cmd == ISO14443A_CMD_SELECT_CL1) {
                /* NXP AN10927 (section 2, figure 1, page 3) shows the expected
                 * data flow to exchange the 7-byte UID for DESFire tags:
                 * http://www.nxp.com/docs/en/application-note/AN10927.pdf
                 */
                ConfigurationUidType Uid;
                ApplicationGetUid(&Uid[1]);
                Uid[0] = ISO14443A_UID0_CT;
                /* Load UID CL1 and perform anticollision: */
                uint8_t cl1SAKValue = SAK_CL1_VALUE;
                if (Buffer[1] == ISO14443A_NVB_AC_START && ISO14443ASelectDesfire(&Buffer[0], &BitCount, &Uid[0], 4, cl1SAKValue)) {
                    ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL1_NVB_END);
                    return BitCount;
                } else if (Buffer[1] == ISO14443A_NVB_AC_END && ISO14443ASelectDesfire(&Buffer[0], &BitCount, &Uid[0], 4, cl1SAKValue)) {
                    ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL2);
                    return BitCount;
                } else {
                    DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL1-NOT-OK"));
                }
            } else {
                DEBUG_PRINT_P(PSTR("ISO14443-4: RDY1 -- NOT-SLCT-CMD"));
            }
            CheckStateRetryCount(false);
            return GetNAKCommandData(Buffer, false);
            //return ISO14443A_APP_NO_RESPONSE;
        }
        case ISO14443_3A_STATE_READY_CL2:
        case ISO14443_3A_STATE_READY_CL2_NVB_END: {
            if (Cmd == ISO14443A_CMD_SELECT_CL2 && ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
                /* Load UID CL2 and perform anticollision: */
                ConfigurationUidType Uid;
                ApplicationGetUid(&Uid[0]);
                uint8_t cl2SAKValue = SAK_CL2_VALUE;
                if (Buffer[1] == ISO14443A_NVB_AC_START && ISO14443ASelectDesfire(&Buffer[0], &BitCount, &Uid[3], 4, cl2SAKValue)) {
                    ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL2_NVB_END);
                    return BitCount;
                } else if (Buffer[1] == ISO14443A_NVB_AC_END && ISO14443ASelectDesfire(&Buffer[0], &BitCount, &Uid[3], 4, cl2SAKValue)) {
                    ISO144433ASwitchState(ISO14443_3A_STATE_ACTIVE);
                    return BitCount;

                } else {
                    DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL2-NOT-OK"));
                }
            } else {
                DEBUG_PRINT_P(PSTR("ISO14443-4: RDY2 -- NOT-SLCT-CMD"));
            }
            CheckStateRetryCount(false);
            return GetNAKCommandData(Buffer, false);
            //return ISO14443A_APP_NO_RESPONSE;
        }
        case ISO14443_3A_STATE_ACTIVE: {
            StateRetryCount = MAX_STATE_RETRY_COUNT;
            if (Cmd == ISO14443A_CMD_RATS) {
                ISO144434SwitchState(ISO14443_4_STATE_EXPECT_RATS);
            } else if (Cmd == ISO14443A_CMD_SELECT_CL3) {
                /* DESFire UID size is of insufficient size to support this request: */
                return GetNAKCommandData(Buffer, false);
            }
            /* Forward to ISO/IEC 14443-4 block processing code */
            return ISO144434ProcessBlock(Buffer, ASBYTES(BitCount), BitCount);
        }
        default:
            break;

    }

    /* Fallthrough: Unknown command. Reset back to idle/halt state. */
    if (!CheckStateRetryCount(false)) {
        DEBUG_PRINT_P(PSTR("ISO14443-3: RST-TO-IDLE 0x%02x"), Cmd);
    }
    return GetNAKCommandData(Buffer, false);
    //return ISO14443A_APP_NO_RESPONSE;

}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
