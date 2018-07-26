//
// Created by Zitai Chen on 25/07/2018.
// Application layer for sniffing
// Currently only support Autocalibrate
//

#include <stdbool.h>
#include <LED.h>
#include "Sniff14443A.h"
#include "Codec/SniffISO14443-2A.h"
extern bool checkParityBits(uint8_t * Buffer, uint16_t BitCount);

Sniff14443Command Sniff14443CurrentCommand = Sniff14443_Do_Nothing;
bool selected = false;
static enum {
    STATE_IDLE,
    STATE_REQA,
    STATE_ATQA,
    STATE_ANTICOLLI,
    STATE_SELECT,
    STATE_UID,
    STATE_SAK,
} SniffState = STATE_IDLE;


void Sniff14443AAppInit(void){
    SniffState = STATE_REQA;
    selected = false;
}

void Sniff14443AAppReset(void){
    SniffState = STATE_IDLE;
    Sniff14443CurrentCommand = Sniff14443_Do_Nothing;
    selected = false;
}
// Currently APPTask and AppTick is not being used
void Sniff14443AAppTask(void){/* Empty */}
void Sniff14443AAppTick(void){/* Empty */}

void Sniff14443AAppTimeout(void){
    Sniff14443AAppReset();
}

void reset2REQA(void){
    SniffState = STATE_REQA;
    LED_PORT.OUTCLR = LED_RED;
    selected = false;
    // TODO: Mark the current threshold as fail and continue

}
uint16_t Sniff14443AAppProcess(uint8_t* Buffer, uint16_t BitCount){
    switch (Sniff14443CurrentCommand){
        case Sniff14443_Do_Nothing:
            return 0;
        case Sniff14443_Autocalibrate:
            switch (SniffState){
                case STATE_REQA:
                    // If received Reader REQA or WUPA
                    if (TrafficSource == TRAFFIC_READER &&
                            (Buffer[0] == 0x26 || Buffer[0] == 0x54))
                    {
                        SniffState = STATE_ATQA;
                    } else {
                        // Stay in this state, do noting
                    }
                    break;
                case STATE_ATQA:
                    // ATQA: P RRRR XXXX  P XXRX XXXX
                    if (TrafficSource == TRAFFIC_CARD &&
                            BitCount == 2*9 &&
                            (Buffer[0] & 0x20) == 0x00 &&        // Bit6 RFU shall be 0
                            (Buffer[1] & 0xE0) == 0x00 &&      // bit13-16 RFU shall be 0
                            (Buffer[2] & 0x01) == 0x00 &&
                            checkParityBits(Buffer, BitCount))
                    {
                        // Assume this is a good ATQA
                        SniffState = STATE_ANTICOLLI;
                    } else {
                        // If not ATQA, but REQA, then stay on this state,
                        // Reset to REQA, save the counter and reset the counter
                        if (TrafficSource == TRAFFIC_READER &&
                            (Buffer[0] == 0x26 || Buffer[0] == 0x54)) {
                        } else {
                            // If not ATQA and not REQA then reset to REQA
                            reset2REQA();
                        }
                    }
                    break;
                case STATE_ANTICOLLI:
                    LED_PORT.OUTSET = LED_RED;
                    // SEL: 93/95/97
                    if(TrafficSource == TRAFFIC_READER &&
                            BitCount == 2*8 &&
                            (Buffer[0] & 0xf0) == 0x90 &&
                            (Buffer[0] & 0x09) == 0x01)
                    {
                        SniffState = STATE_UID;
                    } else{
                        reset2REQA();
                    }
                    break;
                case STATE_UID:
                    if(TrafficSource == TRAFFIC_CARD &&
                       BitCount == 5*9 &&
                       checkParityBits(Buffer, BitCount))
                    {
                        SniffState = STATE_SELECT;
                    } else {
                        reset2REQA();
                    }
                    break;
                case STATE_SELECT:

                    // SELECT: 9 bytes, SEL = 93/95/97, NVB=70
                    if (TrafficSource == TRAFFIC_READER &&
                            BitCount == 9*8 &&
                            (Buffer[0] & 0xf0) == 0x90 &&
                            (Buffer[0] & 0x09) == 0x01 &&
                            Buffer[1] == 0x70)
                    {
                        SniffState = STATE_SAK;
                    } else{
                        // Not valid, reset
                        reset2REQA();
                    }
                    break;
                case STATE_SAK:
                    // SAK: 1Byte SAK + CRC
                    if(TrafficSource == TRAFFIC_CARD &&
                            BitCount == 3*9 &&
                            checkParityBits(Buffer, BitCount))
                    {
                        if((Buffer[0] & 0x04) == 0x00){
                            // UID complete, success SELECTED,
                            // TODO: Mark the current threshold as success and continue
                            // reset
                            SniffState = STATE_REQA;
                            selected = true;
                            LED_PORT.OUTCLR = LED_RED;

                        } else {
                            // UID not complete, goto ANTICOLLI
                            SniffState = STATE_ANTICOLLI;
                        }
                    } else{
                        reset2REQA();
                    }
                    break;
                default:
                    break;
            }
            break;

        default:
            return 0;
    }
}