/*
 * CommandLine.h
 *
 *  Created on: 04.05.2013
 *      Author: skuser
 */

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#include "Terminal.h"

void CommandLineInit(void);
bool CommandLineProcessByte(uint8_t Byte);
void CommandLineTick(void);

void CommandLineAppendData(void* Buffer, uint16_t Bytes);

#endif /* COMMANDLINE_H_ */
