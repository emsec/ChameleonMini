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
uint8_t Iso144434BlockNumber = 0x00;
uint8_t Iso144434CardID = 0x00;
uint8_t Iso144434LastBlockLength = 0x00;
uint8_t StateRetryCount = 0x00;
uint8_t LastReaderSentCmd = 0x00;

bool CheckStateRetryCount2(bool resetByDefault, bool performLogging) {
    if (resetByDefault || ++StateRetryCount >= MAX_STATE_RETRY_COUNT) {
        ISO144434SwitchState2(Iso144433AIdleState, performLogging);
        StateRetryCount = 0x00;
        const char *debugStatusMsg = PSTR("RETRY-RESET");
        LogDebuggingMsg(debugStatusMsg);
        return true;
    }
    return false;
}
bool CheckStateRetryCount(bool resetByDefault) {
    return CheckStateRetryCount2(resetByDefault, true);
}

void ISO144434SwitchState2(Iso144434StateType NewState, bool performLogging) {
    Iso144434State = NewState;
    StateRetryCount = 0x00;
    if (performLogging) {
        DesfireLogISOStateChange(Iso144434State, LOG_ISO14443_4_STATE);
    }
}

void ISO144434SwitchState(Iso144434StateType NewState) {
    ISO144434SwitchState2(NewState, true);
}

void ISO144434Reset(void) {
    /* No logging -- spams the log */
    Iso144434State = ISO14443_4_STATE_EXPECT_RATS;
    Iso144434BlockNumber = 1;
}

static uint16_t ISO144434ProcessBlock(uint8_t *Buffer, uint16_t ByteCount, uint16_t BitCount) {

    uint8_t PCB = Buffer[0];
    uint8_t MyBlockNumber = Iso144434BlockNumber;
    uint8_t PrologueLength;
    uint8_t HaveCID, HaveNAD;

    /* Verify the block's length: at the very least PCB + CRCA */
    if (ByteCount < (1 + ISO14443A_CRCA_SIZE)) {
        /* Huh? Broken frame? */
        const char *debugPrintStr = PSTR("ISO14443-4: length fail");
        LogDebuggingMsg(debugPrintStr);
        return ISO14443A_APP_NO_RESPONSE;
    }
    ByteCount -= 2;

    /* Verify the checksum; fail if doesn't match */
    if (!ISO14443ACheckCRCA(Buffer, ByteCount)) {
        LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, ByteCount);
        /* ISO/IEC 14443-4, clause 7.5.5. The PICC does not attempt any error recovery. */
        const char *debugPrintStr = PSTR("ISO14443-4: CRC fail; %04X vs %04X");
        DEBUG_PRINT_P(debugPrintStr, *(uint16_t *)&Buffer[ByteCount],
                      ISO14443AUpdateCRCA(Buffer, ByteCount, 0x00));
        return ISO14443A_APP_NO_RESPONSE;
    }

    switch (Iso144434State) {
        case ISO14443_4_STATE_EXPECT_RATS: {
            /* See: ISO/IEC 14443-4, clause 5.6.1.2 */
            if (Buffer[0] != ISO14443A_CMD_RATS) {
                /* Ignore blocks other than RATS and HLTA */
                const char *debugPrintStr = PSTR("ISO14443-4: NOT RATS");
                LogDebuggingMsg(debugPrintStr);
                return ISO14443A_APP_NO_RESPONSE;
            }
            /* Process RATS.
             * NOTE: ATS bytes are tailored to Chameleon implementation and differ from DESFire spec.
             * NOTE: Some PCD implementations do a memcmp() over ATS bytes, which is completely wrong.
             */
            Iso144434CardID = Buffer[1] & 0x0F;
            Buffer[0] = 0x06;
            memcpy(&Buffer[1], &Picc.ATSBytes[1], 4);
            Buffer[5] = 0x80; /* T1: dummy value for historical bytes */
            ByteCount = 6;    // NOT including CRC
            ISO144434SwitchState(ISO14443_4_STATE_ACTIVE);
            const char *debugPrintStr = PSTR("ISO14443-4: SEND RATS");
            LogDebuggingMsg(debugPrintStr);
            return GetAndSetBufferCRCA(Buffer, ByteCount);
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
                    /* Different card ID => the frame is ignored */
                    const char *debugPrintStr = PSTR("ISO14443-4: NEW CARD ID");
                    LogDebuggingMsg(debugPrintStr);
                    return ISO14443A_APP_NO_RESPONSE;
                }
            }
        }
        case ISO14443_4_STATE_LAST: {
            return ISO14443A_APP_NO_RESPONSE;
        }
    }

    switch (PCB & ISO14443_PCB_BLOCK_TYPE_MASK) {
        case ISO14443_PCB_I_BLOCK: {
            HaveNAD = PCB & ISO14443_PCB_HAS_NAD_MASK;
            if (HaveNAD) {
                PrologueLength++;
                /* Not currently supported => the frame is ignored */
                const char *debugPrintStr = PSTR("ISO144434ProcessBlock: ISO14443_PCB_I_BLOCK -- %d");
                DEBUG_PRINT_P(debugPrintStr, __LINE__);
            }
            /* 7.5.3.2, rule D: toggle on each I-block */
            Iso144434BlockNumber = MyBlockNumber = !MyBlockNumber;
            if (PCB & ISO14443_PCB_I_BLOCK_CHAINING_MASK) {
                /* Currently not supported => the frame is ignored */
                const char *debugPrintStr = PSTR("ISO144434ProcessBlock: ISO14443_PCB_I_BLOCK -- %d");
                DEBUG_PRINT_P(debugPrintStr, __LINE__);
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
            ByteCount = MifareDesfireProcess(Buffer + PrologueLength, ByteCount - PrologueLength);
            /* Short-circuit in case the app decides not to respond at all */
            if (ByteCount == ISO14443A_APP_NO_RESPONSE) {
                const char *debugPrintStr = PSTR("ISO14443-4: APP_NO_RESP");
                LogDebuggingMsg(debugPrintStr);
                return ISO14443A_APP_NO_RESPONSE;
            }
            ByteCount += PrologueLength;
            const char *debugPrintStr = PSTR("ISO14443-4: I-BLK");
            LogDebuggingMsg(debugPrintStr);
            break;
        }

        case ISO14443_PCB_R_BLOCK: {
            /* 7.5.4.3, rule 11 */
            if ((PCB & ISO14443_PCB_BLOCK_NUMBER_MASK) == MyBlockNumber) {
                /* NOTE: This already includes the CRC */
                const char *debugPrintStr = PSTR("ISO144434ProcessBlock: ISO14443_PCB_R_BLOCK -- %d");
                DEBUG_PRINT_P(debugPrintStr, __LINE__);
                return ISO14443A_APP_NO_RESPONSE;
            }
            if (PCB & ISO14443_PCB_R_BLOCK_ACKNAK_MASK) {
                /* 7.5.4.3, rule 12 */
                /* This is a NAK. Send an ACK back */
                Buffer[0] = ISO14443_PCB_R_BLOCK_STATIC | ISO14443_PCB_R_BLOCK_ACK | MyBlockNumber;
                ByteCount = 1;
            } else {
                /* This is an ACK */
                /* NOTE: Chaining is not supported yet. */
                const char *debugPrintStr = PSTR("ISO144434ProcessBlock: ISO14443_PCB_R_BLOCK -- %d");
                DEBUG_PRINT_P(debugPrintStr, __LINE__);
                return ISO14443A_APP_NO_RESPONSE;
            }
            const char *debugPrintStr6 = PSTR("ISO14443-4: R-BLK");
            LogDebuggingMsg(debugPrintStr6);
            break;
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
                const char *debugPrintStr = PSTR("ISO14443-4: S-BLK");
                LogDebuggingMsg(debugPrintStr);
                return GetAndSetBufferCRCA(Buffer, ByteCount);
            }
            const char *debugPrintStr5 = PSTR("ISO14443-4: PCB_S_BLK NO_RESP");
            LogDebuggingMsg(debugPrintStr5);
            return ISO14443A_APP_NO_RESPONSE;
        }

    }

    /* Fall through (default handling when there is no response to register/return to the sender): */
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
    /* No logging -- spams the log */
    Iso144433AState = ISO14443_3A_STATE_IDLE;
    Iso144433AIdleState = ISO14443_3A_STATE_IDLE;
}

void ISO144433AHalt(void) {
    ISO144433ASwitchState(ISO14443_3A_STATE_HALT);
    Iso144433AIdleState = ISO14443_3A_STATE_HALT;
    StateRetryCount = 0x00;
}

bool ISO144433AIsHalt(const uint8_t *Buffer, uint16_t BitCount) {
    return BitCount == ISO14443A_HLTA_FRAME_SIZE + ISO14443A_CRCA_SIZE * BITS_PER_BYTE
           && Buffer[0] == ISO14443A_CMD_HLTA
           && Buffer[1] == 0x00
           && ISO14443ACheckCRCA(Buffer, (ISO14443A_HLTA_FRAME_SIZE + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
}

uint16_t ISO144433APiccProcess(uint8_t *Buffer, uint16_t BitCount) {

    if (BitCount == 0) {
        return ISO14443A_APP_NO_RESPONSE;
    }

    uint8_t Cmd = Buffer[0];

    /* Wakeup and Request may occure in all states */
    bool checkStateRetryStatus = CheckStateRetryCount(false);
    bool decrementRetryCount = true;
    if ((Cmd == ISO14443A_CMD_REQA) && (LastReaderSentCmd == ISO14443A_CMD_REQA) && !checkStateRetryStatus) {
        /* Catch timing issues where the reader sends multiple
        REQA bytes, in between which we would have already sent
        back a response, so that we should not reset. */
        decrementRetryCount = false;
    } else if (Cmd == ISO14443A_CMD_REQA || Cmd == ISO14443A_CMD_WUPA) {
        ISO144433ASwitchState(ISO14443_3A_STATE_IDLE);
        CheckStateRetryCount(true);
        ISO144434Reset();
        decrementRetryCount = false;
    } else if (ISO144433AIsHalt(Buffer, BitCount)) {
        LogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
        ISO144433ASwitchState(ISO14443_3A_STATE_HALT);
        LastReaderSentCmd = Cmd;
        CheckStateRetryCount(true);
        ISO144434Reset();
        const char *logMsg = PSTR("ISO14443-3: HALTING");
        LogDebuggingMsg(logMsg);
        return ISO14443A_APP_NO_RESPONSE;
    }
    LastReaderSentCmd = Cmd;
    if (decrementRetryCount && StateRetryCount > 0) {
        StateRetryCount -= 1;
    }

    /* This implements ISO 14443-3A state machine */
    /* See: ISO/IEC 14443-3, clause 6.2 */
    switch (Iso144433AState) {
        case ISO14443_3A_STATE_HALT:
            if (Cmd != ISO14443A_CMD_WUPA) {
                const char *debugPrintStr = PSTR("ISO14443-4: HALT / NOT WUPA");
                LogDebuggingMsg(debugPrintStr);
                break;
            } else {
                ISO144433ASwitchState(ISO14443_3A_STATE_IDLE);
            }
        /* Fall-through */

        case ISO14443_3A_STATE_IDLE:
            if (Cmd != ISO14443A_CMD_REQA &&
                    Cmd != ISO14443A_CMD_WUPA) {
                const char *debugPrintStr = PSTR("ISO14443-4: IDLE / NOT WUPA 0x%02x");
                DEBUG_PRINT_P(debugPrintStr);
                break;
            }
            Iso144433AIdleState = Iso144433AState;
            ISO144433ASwitchState(ISO14443_3A_STATE_READY1);
            Buffer[0] = (ATQA_VALUE) & 0x00FF;
            Buffer[1] = (ATQA_VALUE >> 8) & 0x00FF;
            const char *debugPrintStr = PSTR("ISO14443-4 (IDLE): ATQA");
            LogDebuggingMsg(debugPrintStr);
            return ISO14443A_ATQA_FRAME_SIZE_BYTES * BITS_PER_BYTE;

        case ISO14443_3A_STATE_READY1:
            if (Cmd == ISO14443A_CMD_SELECT_CL1) {
                /* Load UID CL1 and perform anticollision. */
                ConfigurationUidType Uid;
                ApplicationGetUid(Uid);
                if (ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
                    Uid[3] = Uid[2];
                    Uid[2] = Uid[1];
                    Uid[1] = Uid[0];
                    Uid[0] = ISO14443A_UID0_CT;
                }
                if (ISO14443ASelectDesfire(Buffer, &BitCount, Uid, SAK_CL1_VALUE)) {
                    /* CL1 stage has ended successfully */
                    const char *debugPrintStr = PSTR("ISO14443-4: Select OK");
                    LogDebuggingMsg(debugPrintStr);
                    ISO144433ASwitchState(ISO14443_3A_STATE_READY2);
                } else {
                    const char *debugPrintStr = PSTR("ISO14443-4: Select NAK");
                    LogDebuggingMsg(debugPrintStr);
                }
                return BitCount;
            }
            const char *debugPrintStr4 = PSTR("ISO14443-4: RDY1, NOT SLCT CMD");
            LogDebuggingMsg(debugPrintStr4);
            break;

        case ISO14443_3A_STATE_READY2:
            if (Cmd == ISO14443A_CMD_SELECT_CL2 && ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
                /* Load UID CL2 and perform anticollision */
                ConfigurationUidType Uid;
                ApplicationGetUid(Uid);
                if (ISO14443ASelectDesfire(Buffer, &BitCount, &Uid[3], SAK_CL2_VALUE)) {
                    /* CL2 stage has ended successfully. This means
                     * our complete UID has been sent to the reader. */
                    ISO144433ASwitchState(ISO14443_3A_STATE_ACTIVE);
                    const char *debugPrintStr = PSTR("INTO ACTIVE!");
                    LogDebuggingMsg(debugPrintStr);
                } else {
                    const char *debugPrintStr = PSTR("Incorrect Select value (R2)");
                    LogDebuggingMsg(debugPrintStr);
                }
                return BitCount;
            }
            const char *debugPrintStr3 = PSTR("RDY2, NOT SLCT CMD");
            LogDebuggingMsg(debugPrintStr3);
            break;

        case ISO14443_3A_STATE_ACTIVE:
            /* Recognise the HLTA command */
            if (ISO144433AIsHalt(Buffer, BitCount)) {
                LogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
                ISO144433ASwitchState(ISO14443_3A_STATE_HALT);
                const char *logMsg = PSTR("ISO14443-3: Got HALT");
                LogDebuggingMsg(logMsg);
                return ISO14443A_APP_NO_RESPONSE;
            }
            /* Forward to ISO/IEC 14443-4 processing code */
            uint16_t ByteCount = (BitCount + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
            uint16_t ReturnBits = ISO144434ProcessBlock(Buffer, ByteCount, BitCount);
            const char *debugPrintStr2 = PSTR("ISO14443-4: ACTIVE RET");
            LogDebuggingMsg(debugPrintStr2);
            return ReturnBits;

        default:
            break;

    }

    /* Unknown command. Reset back to idle/halt state. */
    bool defaultReset = false; //Iso144433AState != ISO14443_3A_STATE_IDLE;
    if (!CheckStateRetryCount(defaultReset)) {
        const char *logMsg = PSTR("ISO14443-3: RESET TO IDLE 0x%02x");
        DEBUG_PRINT_P(logMsg, Cmd);
        return ISO14443A_APP_NO_RESPONSE;
    }
    const char *debugPrintStr = PSTR("ISO14443-4: UNK-CMD NO_RESP");
    LogDebuggingMsg(debugPrintStr);
    return ISO14443A_APP_NO_RESPONSE;

}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
