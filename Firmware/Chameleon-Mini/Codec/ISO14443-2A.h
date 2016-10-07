/*
 * ISO14443-2A.h
 *
 *  Created on: 18.02.2013
 *      Author: skuser
 */

#ifndef ISO14443_2A_H_
#define ISO14443_2A_H_

#include "Codec.h"

#define ISO14443A_APP_NO_RESPONSE       0x0000
#define ISO14443A_APP_CUSTOM_PARITY     0x1000

#define ISO14443A_BUFFER_PARITY_OFFSET    (CODEC_BUFFER_SIZE/2)

/* Codec Interface */
void ISO14443ACodecInit(void);
void ISO14443ACodecDeInit(void);
void ISO14443ACodecTask(void);



#endif
