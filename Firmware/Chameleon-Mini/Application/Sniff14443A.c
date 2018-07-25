//
// Created by Zitai Chen on 25/07/2018.
// Application layer for sniffing
// Currently only support Autocalibrate
//

#include <stdbool.h>
#include <LED.h>
#include "Sniff14443A.h"
Sniff14443Command Sniff14443CurrentCommand = Sniff14443_Do_Nothing;

extern bool checkParityBits(uint8_t * Buffer, uint16_t BitCount);
        static enum {
    STATE_IDLE,
    STATE_READER_DATA,
    STATE_CARD_DATA
} SniffState = STATE_IDLE;

void Sniff14443AAppInit(void){
    SniffState = STATE_IDLE;
}

void Sniff14443AAppReset(void){
    SniffState = STATE_IDLE;
    Sniff14443CurrentCommand = Sniff14443_Do_Nothing;
}
// Currently APPTask and AppTick is not being used
void Sniff14443AAppTask(void){/* Empty */}
void Sniff14443AAppTick(void){/* Empty */}

void Sniff14443AAppTimeout(void){
    Sniff14443AAppReset();
}

uint16_t Sniff14443AAppProcess(uint8_t* Buffer, uint16_t BitCount){
    switch (Sniff14443CurrentCommand){
        case Sniff14443_Do_Nothing:
            return 0;
        case Sniff14443_Autocalibrate:
            // If received Reader WUPA
            if(Buffer[0] == 0x26 || Buffer[0] == 0x54){
                LED_PORT.OUTSET = LED_RED;
            }
            break;
        default:
            return 0;
    }
}