/*
 * SniffISO15693.h
 *
 *  Created on: 05.11.2019
 *      Author: ceres-c
 */

#ifdef CONFIG_ISO15693_SNIFF_SUPPORT

#include "../Codec/SniffISO15693.h"
#include "Sniff15693.h"

Sniff15693Command Sniff15693CurrentCommand;

void SniffISO15693AppTimeout(void)
{
    SniffISO15693AppReset();
}

void SniffISO15693AppInit(void)
{
    Sniff15693CurrentCommand = Sniff15693_Do_Nothing;
}

void SniffISO15693AppReset(void)
{
    SniffISO15693AppInit();
}


void SniffISO15693AppTask(void)
{
    
}

void SniffISO15693AppTick(void)
{

}

uint16_t SniffISO15693AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes)
{
    char str[64];
    switch (Sniff15693CurrentCommand) {
        case Sniff15693_Do_Nothing: {
            return 0;
        }
        case Sniff15693_Autocalibrate: {
            sprintf(str, "CMD: Sniff15693_Autocalibrate ");
            LogEntry(LOG_INFO_GENERIC, str, strlen(str));
            CommandLinePendingTaskFinished(COMMAND_INFO_FALSE_ID, NULL);
            return 0;
        }
    }
    return 0;
}

#endif /* CONFIG_ISO15693_SNIFF_SUPPORT */
