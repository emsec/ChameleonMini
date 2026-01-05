/*
 * ISO14443-2F.h
 *
 *  Created on: 5.7.2024
 *      Author: Ladislav Marko
 *  Inspired by ISO14443-2A.h and ISO15693.h
 */

#ifndef ISO14443_F_H_
#define ISO14443_F_H_
#include "Codec.h"

#define ISO14443F_APP_NO_RESPONSE       0x0000

/* Codec Interface */
void ISO14443FCodecInit(void);
void ISO14443FCodecDeInit(void);
void ISO14443FCodecTask(void);

#endif //ISO14443_F_H_
