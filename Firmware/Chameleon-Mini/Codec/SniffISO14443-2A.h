//
// Created by 陈子泰 on 05/07/2018.
//

#ifndef CHAMELEON_MINI_SNIFFISO14443_2A_H
#define CHAMELEON_MINI_SNIFFISO14443_2A_H


#include "Codec.h"
#include "Terminal/CommandLine.h"

/* Codec Interface */
void Sniff14443ACodecInit(void);
void Sniff14443ACodecDeInit(void);
void Sniff14443ACodecTask(void);



#endif //CHAMELEON_MINI_SNIFFISO14443_2A_H
