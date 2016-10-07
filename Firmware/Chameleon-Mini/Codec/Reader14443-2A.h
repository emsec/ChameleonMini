/*
 * Reader14443-2A.h
 *
 *  Created on: 26.08.2014
 *      Author: sk
 */

#ifndef READER14443_2A_H_
#define READER14443_2A_H_

#include "Codec.h"
#include "Terminal/CommandLine.h"

/* Codec Interface */
void Reader14443ACodecInit(void);
void Reader14443ACodecDeInit(void);
void Reader14443ACodecTask(void);

/* Application Interface */
void Reader14443ACodecStart(void);
void Reader14443ACodecReset(void);

#endif /* READER14443_2A_H_ */
