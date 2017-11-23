/*
 * Random.h
 *
 *  Created on: 22.03.2013
 *      Author: skuser
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include "Common.h"

void RandomInit(void);
uint8_t RandomGetByte(void);
void RandomGetBuffer(void* Buffer, uint8_t ByteCount);
void RandomTick(void);

#endif /* RANDOM_H_ */
