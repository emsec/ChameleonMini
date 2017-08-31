/*
 * ISO15693.c
 *
 *  Created on: 01-03-2017
 *      Author: Phillip Nash
 */


#include "Vicinity.h"
#include "../Codec/ISO15693.h"
#include "../Memory.h"
#include "Crypto1.h"
#include "../Random.h"
#include "ISO15693-A.h"


static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;

ISO15693UidType MyUid = {0x60, 0x0B, 0x88, 0x77, 0x06, 0x44, 0x02, 0xE1};



void VicinityAppInit(void)
{
    State = STATE_READY;
}

void VicinityAppReset(void)
{
    State = STATE_READY;
}


void VicinityAppTask(void)
{
    
}

void VicinityAppTick(void)
{

    
}

void sendIntToTermina(uint8_t val)
{
    char buf[16];
    snprintf(buf,16, "%d,",val);
    TerminalSendString(buf);
}

uint16_t VicinityAppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    if (FrameBytes >= ISO15693_MIN_FRAME_SIZE) {
        if(ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE)) {
            // At this point, we have a valid ISO15693 frame
            uint8_t Command = FrameBuf[1];
            uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
            
            
            switch(State) {
            case STATE_READY:
                if (Command == ISO15693_CMD_INVENTORY) {
                    FrameBuf[0] = 0x00; /* Flags */
                    FrameBuf[1] = 0x00; /* DSFID */
                    ISO15693CopyUid(&FrameBuf[2], MyUid);
                    ResponseByteCount = 10;
                } else if (Command == ISO15693_CMD_STAY_QUIET) {
                    if (ISO15693Addressed(FrameBuf, MyUid)) {
                        State = STATE_QUIET;
                    }
                } else if (Command == ISO15693_CMD_GET_SYS_INFO) {
                    if (ISO15693Addressed(FrameBuf, MyUid)) {
                        FrameBuf[0] = 0; /* Flags */
                        FrameBuf[1] = 0; /* InfoFlags */
                        ISO15693CopyUid(&FrameBuf[2], MyUid);
                        ResponseByteCount = 10;
                    }
                }
                break;

            case STATE_SELECTED:

                break;

            case STATE_QUIET:
                if (Command == ISO15693_CMD_RESET_TO_READY) {
                    if (ISO15693Addressed(FrameBuf, MyUid)) {
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

void VicinityGetUid(ConfigurationUidType Uid)
{
    memcpy(Uid, MyUid, sizeof(ConfigurationUidType));
}

void VicinitySetUid(ConfigurationUidType Uid)
{
    memcpy(MyUid, Uid, sizeof(ConfigurationUidType));
}



