/*
 * ISO15693.h
 *
 *  Created on: 25.01.2017
 *      Author: Phillip Nash
 */

#ifndef ISO15693_H_
#define ISO15693_H_

#define ISO15693_APP_NO_RESPONSE        0x0000

/* Codec Interface */
void ISO15693CodecInit(void);
void ISO15693CodecDeInit(void);
void ISO15693CodecTask(void);

/* Application Interface */
void ISO15693CodecStart(void);
void ISO15693CodecReset(void);

#endif  /* ISO15693_H_ */
