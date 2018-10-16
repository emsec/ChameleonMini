//
// Created by Zitai Chen on 25/07/2018.
//

#ifndef CHAMELEON_MINI_SNIFF14443A_H
#define CHAMELEON_MINI_SNIFF14443A_H

#include <stdint.h>

void Sniff14443AAppInit(void);
void Sniff14443AAppReset(void);
void Sniff14443AAppTask(void);
void Sniff14443AAppTick(void);
void Sniff14443AAppTimeout(void);

uint16_t Sniff14443AAppProcess(uint8_t* Buffer, uint16_t BitCount);

typedef enum {
    Sniff14443_Do_Nothing,
    Sniff14443_Autocalibrate,
} Sniff14443Command;

#endif //CHAMELEON_MINI_SNIFF14443A_H

