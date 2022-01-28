/*
 * SniffISO15693.h
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
uint16_t SniffISO15693AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes);

#endif /* SNIFF_15693_H_ */
