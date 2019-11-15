/*
 * ISO15693.c
 *
 *  Created on: 01-03-2017
 *      Author: Phillip Nash
 *
 *  TODO:
 *    - ISO15693AddressedLegacy should be replaced with ISO15693Addressed and appropriate check
 *      should be performed (see TITagitstandard.c) - ceres-c
 */


#include "Vicinity.h"
#include "../Codec/ISO15693.h"
#include "../Memory.h"
#include "Crypto1.h"
#include "../Random.h"
#include "ISO15693-A.h"

#define MEM_UID_ADDRESS         0x00

static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

void VicinityAppInit(void) {
    State = STATE_READY;
}

void VicinityAppReset(void) {
    State = STATE_READY;
}


void VicinityAppTask(void) {

}

void VicinityAppTick(void) {


}

uint16_t VicinityAppProcess(uint8_t *FrameBuf, uint16_t FrameBytes) {
    if (FrameBytes >= ISO15693_MIN_FRAME_SIZE) {
        if (ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE)) {
            // At this point, we have a valid ISO15693 frame
            uint8_t Command = FrameBuf[1];
            uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
            uint8_t Uid[8];
            MemoryReadBlock(Uid, MEM_UID_ADDRESS, ActiveConfiguration.UidSize);

            switch (State) {
                case STATE_READY:
                    if (Command == ISO15693_CMD_INVENTORY) {
                        FrameBuf[0] = 0x00; /* Flags */
                        FrameBuf[1] = 0x00; /* DSFID */
                        ISO15693CopyUid(&FrameBuf[2], Uid);
                        ResponseByteCount = 10;
                    } else if (Command == ISO15693_CMD_STAY_QUIET) {
                        if (ISO15693AddressedLegacy(FrameBuf, Uid)) {
                            State = STATE_QUIET;
                        }
                    } else if (Command == ISO15693_CMD_GET_SYS_INFO) {
                        if (ISO15693AddressedLegacy(FrameBuf, Uid)) {
                            FrameBuf[0] = 0; /* Flags */
                            FrameBuf[1] = 0; /* InfoFlags */
                            ISO15693CopyUid(&FrameBuf[2], Uid);
                            ResponseByteCount = 10;
                        }
                    }
                    break;

                case STATE_SELECTED:

                    break;

                case STATE_QUIET:
                    if (Command == ISO15693_CMD_RESET_TO_READY) {
                        MemoryReadBlock(Uid, MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
                        if (ISO15693AddressedLegacy(FrameBuf, Uid)) {
                            FrameBuf[0] = 0;
                            ResponseByteCount = 1;
                            State = STATE_READY;
                        }
                    }
                    break;

                default:
                    break;
            }

            if (ResponseByteCount > 0) {
                /* There is data to be sent. Append CRC */
                ISO15693AppendCRC(FrameBuf, ResponseByteCount);
                ResponseByteCount += ISO15693_CRC16_SIZE;
            }

            return ResponseByteCount;

        } else { // Invalid CRC
            return ISO15693_APP_NO_RESPONSE;
        }
    } else { // Min frame size not met
        return ISO15693_APP_NO_RESPONSE;
    }

}

void VicinityGetUid(ConfigurationUidType Uid) {
    MemoryReadBlock(&Uid[0], MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}

void VicinitySetUid(ConfigurationUidType Uid) {
    MemoryWriteBlock(Uid, MEM_UID_ADDRESS, ActiveConfiguration.UidSize);
}



