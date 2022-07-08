/*
 * Sniff15693.h
 *
 *  Created on: 05.11.2019
 *      Author: ceres-c
 */

#ifndef SNIFF_15693_H_
#define SNIFF_15693_H_

#include "Application.h"
#include "ISO15693-A.h"

void SniffISO15693AppInit(void);
void SniffISO15693AppReset(void);
void SniffISO15693AppTask(void);
void SniffISO15693AppTick(void);
uint16_t SniffISO15693AppProcess(uint8_t *FrameBuf, uint16_t FrameBytes);
void SniffISO15693AppTimeout(void);

typedef enum {
    Sniff15693_Do_Nothing,
    Sniff15693_Autocalibrate,
} Sniff15693Command;

extern Sniff15693Command Sniff15693CurrentCommand;

#endif /* SNIFF_15693_H_ */
