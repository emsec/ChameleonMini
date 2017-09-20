/*
 * ISO15693.c
 *
 *  Created on: 01-03-2017
 *      Author: Phillip Nash
 */


#include "Sl2s2002.h"
#include "../Codec/ISO15693.h"
#include "../Memory.h"
#include "Crypto1.h"
#include "../Random.h"
#include "ISO15693-A.h"

#define BYTES_PER_PAGE        4

static enum {
    STATE_READY,
    STATE_SELECTED,
    STATE_QUIET
} State;


ISO15693UidType Sl2s2002Uid = {0x78, 0xD3, 0xF4, 0x3F, 0x50, 0x01, 0x04, 0xE0};

//ISO15693UidType Sl2s2002Uid = {0x60, 0x0B, 0x88, 0x77, 0x06, 0x44, 0x02, 0xE1};



void Sl2s2002AppInit(void)
{
    State = STATE_READY;
}

void Sl2s2002AppReset(void)
{
    State = STATE_READY;
}


void Sl2s2002AppTask(void)
{
    
}

void Sl2s2002AppTick(void)
{

    
}

// void sendIntToTermina(uint8_t val)
// {
//     char buf[16];
//     snprintf(buf,16, "%02x",val);
//     TerminalSendString(buf);
// }

uint16_t Sl2s2002AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    if (FrameBytes >= ISO15693_MIN_FRAME_SIZE) {
        if(ISO15693CheckCRC(FrameBuf, FrameBytes - ISO15693_CRC16_SIZE)) {
            // At this point, we have a valid ISO15693 frame
            uint8_t Command = FrameBuf[1];
            uint16_t ResponseByteCount = ISO15693_APP_NO_RESPONSE;
            
            //TerminalSendString("Received command: ");
//            for (int i = 0; i < (FrameBytes - 2); i++) {
//                sendIntToTermina(FrameBuf[i]);
//            }
//            TerminalSendString("\n");
            
            
            switch(State) {
            case STATE_READY:
                if (Command == ISO15693_CMD_INVENTORY) {
                    FrameBuf[0] = 0x00; /* Flags */
                    FrameBuf[1] = 0x00; /* DSFID */
                    ISO15693CopyUid(&FrameBuf[2], Sl2s2002Uid);
                    ResponseByteCount = 10;
                } else if (Command == ISO15693_CMD_STAY_QUIET) {
                    if (ISO15693Addressed(FrameBuf, Sl2s2002Uid)) {
                        State = STATE_QUIET;
                    }
                } else if (Command == ISO15693_CMD_GET_SYS_INFO) {
                    if (ISO15693Addressed(FrameBuf, Sl2s2002Uid)) {
                        FrameBuf[0] = 0; /* Flags */
                        FrameBuf[1] = 0x0F; /* InfoFlags */
                        ISO15693CopyUid(&FrameBuf[2], Sl2s2002Uid);
                        FrameBuf[10] = 0x00;
                        FrameBuf[11] = 0xC2;
                        FrameBuf[12] = 0x04;
                        FrameBuf[13] = 0x03;
                        FrameBuf[14] = 0x01;
                        ResponseByteCount = 15;
                    }
                } else if (Command == ISO15693_CMD_READ_SINGLE) {
                    if (ISO15693Addressed(FrameBuf, Sl2s2002Uid)) {
                        uint8_t PageAddress = FrameBuf[10];
                        FrameBuf[0] = 0; /* Flags */
                        MemoryReadBlock(FrameBuf + 1, PageAddress * BYTES_PER_PAGE, BYTES_PER_PAGE);
                        ResponseByteCount = 5;
                    }
                } else if (Command == ISO15693_CMD_READ_MULTIPLE) {
                    if (ISO15693Addressed(FrameBuf, Sl2s2002Uid)) {
                        uint8_t PageAddressStart = FrameBuf[10];
                        uint8_t PageAddressEnd = FrameBuf[11];
                        FrameBuf[0] = 0; /* Flags */
                        MemoryReadBlock(FrameBuf + 1, PageAddressStart * BYTES_PER_PAGE, BYTES_PER_PAGE * (PageAddressEnd - PageAddressStart + 1));
                        ResponseByteCount = 1 + BYTES_PER_PAGE * (PageAddressEnd - PageAddressStart + 1);
                    }
                } else if (Command == ISO15693_CMD_GET_BLOCK_SEC) {
                    if (ISO15693Addressed(FrameBuf, Sl2s2002Uid)) {                        
                        uint8_t PageAddressStart = FrameBuf[10];
                        uint8_t PageAddressEnd = FrameBuf[11];
                        FrameBuf[0] = 0; /* Flags */
                        for (int i = 1; i <= (PageAddressEnd - PageAddressStart + 1); i++) {
                            FrameBuf[i] = 0x00;
                        }
                        ResponseByteCount = 1 + (PageAddressEnd - PageAddressStart + 1);
                    }
                }
                break;
            case STATE_SELECTED:

                break;

            case STATE_QUIET:
                if (Command == ISO15693_CMD_RESET_TO_READY) {
                    if (ISO15693Addressed(FrameBuf, Sl2s2002Uid)) {
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

void Sl2s2002GetUid(ConfigurationUidType Uid)
{
    memcpy(Uid, Sl2s2002Uid, sizeof(ConfigurationUidType));
}

void Sl2s2002SetUid(ConfigurationUidType Uid)
{
    memcpy(Sl2s2002Uid, Uid, sizeof(ConfigurationUidType));
}



