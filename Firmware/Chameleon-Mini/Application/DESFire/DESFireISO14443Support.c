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
    Iso144434State = ISO14443_4_STATE_EXPECT_RATS;
    Iso144434BlockNumber = 1;
    ISO14443ALastIncomingDataFrameBits = 0;
    ISO14443ALastIncomingDataFrame[0] = 0x00;
}

uint16_t ISO144434ProcessBlock(uint8_t *Buffer, uint16_t ByteCount, uint16_t BitCount) {

    uint8_t PCB = Buffer[0];
    uint8_t MyBlockNumber = Iso144434BlockNumber;
    uint8_t PrologueLength = 0;
    uint8_t HaveCID = 0, HaveNAD = 0;

    /* Verify the block's length: at the very least PCB + CRCA */
    if (ByteCount < (1 + ISO14443A_CRCA_SIZE)) {
        /*  Broken frame -- Respond error by returning an empty frame */
        DEBUG_PRINT_P(PSTR("ISO14443-4: length fail"));
        return ISO14443A_APP_NO_RESPONSE;
    }
    ByteCount -= 2;

    /* Verify the checksum; fail if doesn't match */
    if (!ISO14443ACheckCRCA(Buffer, ByteCount)) {
        DesfireLogEntry(LOG_ERR_APP_CHECKSUM_FAIL, (uint8_t *) NULL, 0);
        /* ISO/IEC 14443-4, clause 7.5.5. The PICC does not attempt any error recovery. */
        DEBUG_PRINT_P(PSTR("WARN: 14443-4: CRC fail"));
        return ISO14443A_APP_NO_RESPONSE;
    }

    switch (Iso144434State) {

        case ISO14443_4_STATE_EXPECT_RATS: {
            /* See: ISO/IEC 14443-4, clause 5.6.1.2 */
            if (Buffer[0] != ISO14443A_CMD_RATS) {
                /* Ignore blocks other than RATS and HLTA */
                DEBUG_PRINT_P(PSTR("ISO14443-4: NOT RATS"));
                return ISO14443A_APP_NO_RESPONSE;
            }
            /* Process RATS.
             * NOTE: ATS bytes are tailored to Chameleon implementation and differ from DESFire spec.
             * NOTE: Some PCD implementations do a memcmp() over ATS bytes, which is completely wrong.
             */
            //DEBUG_PRINT_P(PSTR("ISO14443-4: SEND RATS"));
            Iso144434CardID = Buffer[1] & 0x0F;
            Buffer[0] = 0x06;
            memcpy(&Buffer[1], &Picc.ATSBytes[1], 4);
            Buffer[5] = 0x80; /* T1: dummy value for historical bytes */
            ByteCount = 6;    /* NOT including CRC */
            ISO144434SwitchState(ISO14443_4_STATE_ACTIVE);
            return ASBITS(ByteCount); /* PM3 expects no CRCA bytes */
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
                if ((Buffer[1] & 0xF) != Iso144434CardID) {
                    /* Different card ID -- the frame is ignored */
                    DEBUG_PRINT_P(PSTR("ISO14443-4: NEW CARD ID %02d"), Iso144434CardID);
                    return ISO14443A_APP_NO_RESPONSE;
                }
            }
            break;
        }
        case ISO14443_4_STATE_LAST: {
            return ISO14443A_APP_NO_RESPONSE;
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
                DEBUG_PRINT_P(PSTR("ISO144434ProcessBlock: ISO14443_PCB_I_BLOCK"));
            }
            /* 7.5.3.2, rule D: toggle on each I-block */
            Iso144434BlockNumber = MyBlockNumber = !MyBlockNumber;
            if (PCB & ISO14443_PCB_I_BLOCK_CHAINING_MASK) {
                /* Currently not supported -- the frame is ignored */
                DEBUG_PRINT_P(PSTR("ISO144434ProcessBlock: ISO14443_PCB_I_BLOCK"));
                return ISO14443A_APP_NO_RESPONSE;
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
                DEBUG_PRINT_P(PSTR("ISO14443-4: APP_NO_RESP"));
                return ISO14443A_APP_NO_RESPONSE;
            }
            ByteCount += PrologueLength;
            DEBUG_PRINT_P(PSTR("ISO14443-4: I-BLK"));
            return GetAndSetBufferCRCA(Buffer, ByteCount);
        }

        case ISO14443_PCB_R_BLOCK: {
            /* 7.5.4.3, rule 11 */
            if ((PCB & ISO14443_PCB_BLOCK_NUMBER_MASK) == MyBlockNumber) {
                DEBUG_PRINT_P(PSTR("ISO144434ProcessBlock: ISO14443_PCB_R_BLOCK"));
                return ISO14443A_APP_NO_RESPONSE;
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
                DEBUG_PRINT_P(PSTR("ISO144434ProcessBlock: ISO14443_PCB_R_BLOCK"));
                // Resend the data from the last frame:
                if (ISO14443ALastIncomingDataFrameBits > 0) {
                    memcpy(&Buffer[0], &ISO14443ALastIncomingDataFrame[0], ASBYTES(ISO14443ALastIncomingDataFrameBits));
                    return ISO14443ALastIncomingDataFrameBits;
                } else {
                    return ISO14443A_APP_NO_RESPONSE;
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
            DEBUG_PRINT_P(PSTR("ISO14443-4: PCB_S_BLK NO_RESP"));
            return ISO14443A_APP_NO_RESPONSE;
        }

        default:
            break;

    }

    /* Fall through: */
    return ISO14443A_APP_NO_RESPONSE;

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
    ISO144433ASwitchState(ISO14443_3A_STATE_HALT);
    Iso144433AIdleState = ISO14443_3A_STATE_HALT;
    ISO144433AReset();
}

bool ISO144433AIsHalt(const uint8_t *Buffer, uint16_t BitCount) {
    return BitCount == ISO14443A_HLTA_FRAME_SIZE + ASBITS(ISO14443A_CRCA_SIZE) &&
           Buffer[0] == ISO14443A_CMD_HLTA &&
           Buffer[1] == 0x00 &&
           ISO14443ACheckCRCA(Buffer, ASBYTES(ISO14443A_HLTA_FRAME_SIZE));
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
        DEBUG_PRINT_P(PSTR("ISO14443-3: HALTING"));
        ISO144433AHalt();
        return ISO14443A_APP_NO_RESPONSE;
    } else if (CheckStateRetryCount(false)) {
        /* ??? TODO: Is this the correct action ??? */
        DEBUG_PRINT_P(PSTR("ISO14443-3: RESETTING"));
        ISO144433AHalt();
        Buffer[0] == ISO14443A_CMD_HLTA;
        Buffer[1] == 0x00;
        ISO14443AAppendCRCA(Buffer, ASBYTES(ISO14443A_HLTA_FRAME_SIZE));
        return ISO14443A_HLTA_FRAME_SIZE + ASBITS(ISO14443A_CRCA_SIZE);
        //return ISO14443A_APP_NO_RESPONSE;
    }

    /* This implements ISO 14443-3A state machine */
    /* See: ISO/IEC 14443-3, clause 6.2 */
    switch (Iso144433AState) {

        case ISO14443_3A_STATE_HALT:
            if (!ISO14443ACmdIsWUPA(Cmd)) {
                DEBUG_PRINT_P(PSTR("ISO14443-4: HALT -- NOT WUPA"));
                break;
            } else {
                ISO144433ASwitchState(ISO14443_3A_STATE_IDLE);
            }

        case ISO14443_3A_STATE_IDLE:
            Iso144433AIdleState = Iso144433AState;
            ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL1);
            Buffer[0] = DesfireATQAValue & 0x00FF;
            Buffer[1] = (DesfireATQAValue >> 8) & 0x00FF;
            return ASBITS(ISO14443A_ATQA_FRAME_SIZE_BYTES);

        case ISO14443_3A_STATE_READY_CL1:
        case ISO14443_3A_STATE_READY_CL1_NVB_END:
            if (Cmd == ISO14443A_CMD_SELECT_CL1) {
                /* Load UID CL1 and perform anticollisio: */
                ConfigurationUidType Uid;
                ApplicationGetUid(Uid);
                if (ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
                    Uid[0] = ISO14443A_UID0_CT;
                }
                uint8_t cl1SAKValue = IS_ISO14443A_4_COMPLIANT(Buffer[1]) ? ISO14443A_SAK_INCOMPLETE : ISO14443A_SAK_INCOMPLETE_NOT_COMPLIANT;
                Buffer[1] = MAKE_ISO14443A_4_COMPLIANT(Buffer[1]);
                if (Buffer[1] == ISO14443A_NVB_AC_START && !ISO14443ASelectDesfire(Buffer, &BitCount, &Uid[0], 4, cl1SAKValue) && BitCount > 0) {
                    //DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL1 NVB START -- OK"));
                    ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL1_NVB_END);
                    return BitCount;
                } else if (Buffer[1] == ISO14443A_NVB_AC_END && ISO14443ASelectDesfire(Buffer, &BitCount, &Uid[0], 4, cl1SAKValue)) {
                    //DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL1 NVB END -- OK"));
                    ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL2);
                    return BitCount;

                } else {
                    DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL1 NOT OK"));
                }
            } else {
                DEBUG_PRINT_P(PSTR("ISO14443-4: RDY1 -- NOT SLCT CMD"));
            }
            CheckStateRetryCount(false);
            return ISO14443A_APP_NO_RESPONSE;

        case ISO14443_3A_STATE_READY_CL2:
        case ISO14443_3A_STATE_READY_CL2_NVB_END:
            if (Cmd == ISO14443A_CMD_SELECT_CL2 && ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
                /* Load UID CL2 and perform anticollision: */
                ConfigurationUidType Uid;
                ApplicationGetUid(Uid);
                uint8_t cl2SAKValue = IS_ISO14443A_4_COMPLIANT(Buffer[1]) ? ISO14443A_SAK_COMPLETE_COMPLIANT : ISO14443A_SAK_COMPLETE_NOT_COMPLIANT;
                Buffer[1] = MAKE_ISO14443A_4_COMPLIANT(Buffer[1]);
                if (Buffer[1] == ISO14443A_NVB_AC_START && !ISO14443ASelectDesfire(Buffer, &BitCount, &Uid[4], 3, cl2SAKValue) && BitCount > 0) {
                    //DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL2 NVB START -- OK"));
                    ISO144433ASwitchState(ISO14443_3A_STATE_READY_CL2_NVB_END);
                    return BitCount;
                } else if (Buffer[1] == ISO14443A_NVB_AC_END && ISO14443ASelectDesfire(Buffer, &BitCount, &Uid[4], 3, cl2SAKValue)) {
                    //DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL2 NVB END -- OK"));
                    ISO144433ASwitchState(ISO14443_3A_STATE_ACTIVE);
                    return BitCount;

                } else {
                    DEBUG_PRINT_P(PSTR("ISO14443-4: Select CL2 NOT OK"));
                }
            } else {
                DEBUG_PRINT_P(PSTR("ISO14443-4: RDY2 -- NOT SLCT CMD"));
            }
            CheckStateRetryCount(false);
            return ISO14443A_APP_NO_RESPONSE;

        case ISO14443_3A_STATE_ACTIVE:
            StateRetryCount = MAX_STATE_RETRY_COUNT;
            if (ISO144433AIsHalt(Buffer, BitCount)) {
                /* Recognise the HLTA command: */
                DesfireLogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
                ISO144433AHalt();
                return ISO14443A_APP_NO_RESPONSE;
            } else if (Cmd == ISO14443A_CMD_RATS) {
                //DEBUG_PRINT_P(PSTR("ISO14443-3/4: Expecting RATS"));
                ISO144434SwitchState(ISO14443_4_STATE_EXPECT_RATS);
            } else if (Cmd == ISO14443A_CMD_SELECT_CL3) {
                /* DESFire UID size is of insufficient size to support this: */
                Buffer[0] = ISO14443A_SAK_COMPLETE_NOT_COMPLIANT;
                ISO14443AAppendCRCA(&Buffer[0], 1);
                return ISO14443A_SAK_FRAME_SIZE;
            } else if (Cmd == ISO14443A_CMD_DESELECT) {
                /* This has been observed to happen at this stage when swiping the
                 * Chameleon running CONFIG=MF_DESFIRE on an ACR122 USB external reader.
                 * ??? TODO: What are we supposed to return in this case ???
                 */
                DesfireLogEntry(LOG_INFO_APP_CMD_DESELECT, NULL, 0);
                //return ISO14443A_APP_NO_RESPONSE;
                return BitCount;
            }
            /* Forward to ISO/IEC 14443-4 processing code */
            //DEBUG_PRINT_P(PSTR("ISO14443-4: ACTIVE RET"));
            uint16_t ByteCount = ASBYTES(BitCount);
            uint16_t ReturnBits = ISO144434ProcessBlock(Buffer, ByteCount, BitCount);
            return ReturnBits;

        default:
            break;

    }

    /* Fallthrough: Unknown command. Reset back to idle/halt state. */
    if (!CheckStateRetryCount(false)) {
        DEBUG_PRINT_P(PSTR("ISO14443-3: Fall through -- RESET TO IDLE 0x%02x"), Cmd);
        return ISO14443A_APP_NO_RESPONSE;
    } else {
        DEBUG_PRINT_P(PSTR("ISO14443-4: UNK-CMD NO RESP"));
        return ISO14443A_APP_NO_RESPONSE;
    }

}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
