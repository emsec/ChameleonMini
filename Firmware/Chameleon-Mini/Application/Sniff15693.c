/*
 * SniffISO15693.h
 *
 *  Created on: 05.11.2019
 *      Author: ceres-c
 */

#ifdef CONFIG_ISO15693_SNIFF_SUPPORT

#include "../Codec/SniffISO15693.h"
#include "Sniff15693.h"

void SniffISO15693AppInit(void)
{
}

void SniffISO15693AppReset(void)
{
}


void SniffISO15693AppTask(void)
{
    
}

void SniffISO15693AppTick(void)
{

    
}

uint16_t SniffISO15693AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    return 0;
}

#endif /* CONFIG_ISO15693_SNIFF_SUPPORT */
