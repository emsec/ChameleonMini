/*
 * CommandLine.h
 *
 *  Created on: 04.05.2013
 *      Author: skuser
 */

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#include "Terminal.h"
#include "Commands.h"

void CommandLineInit(void);
bool CommandLineProcessByte(uint8_t Byte);
void CommandLineTick(void);

void CommandLineAppendData(void const * const Buffer, uint16_t Bytes);

/* Functions for timeout commands */
void CommandLinePendingTaskFinished(CommandStatusIdType ReturnStatusID, char const * const OutMessage); // must be called, when the intended task is finished
extern void (*CommandLinePendingTaskTimeout) (void); // gets called on timeout to end the pending task
void CommandLinePendingTaskBreak(void); // this manually triggers a timeout

#endif /* COMMANDLINE_H_ */
