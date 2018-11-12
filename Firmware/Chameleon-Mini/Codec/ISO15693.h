/*
 * ISO15693.h
 *
 *  Created on: 25.01.2017
 *      Author: Phillip Nash
 */

#ifndef ISO15693_H_
#define ISO15693_H_

#include "Terminal/CommandLine.h"

#define ISO15693_APP_NO_RESPONSE        0x0000

/* VERY OUTDATED */
#define TIMER_SAMPLING                  TCC0
#define TIMER_T1_BITRATE                TCD1
#define TIMER_SUBCARRIER                TCD0

#define SUBCARRIER_1                    32
#define SUBCARRIER_2                    28
#define SUBCARRIER_OFF                  0
#define SOF_PATTERN                     0x1D
#define EOF_PATTERN                     0xB8

#define T1_NOMINAL_CYCLES               4352 - 14//4352 - 25 /* ISR prologue compensation */
#define WRITE_GRID_CYCLES               4096

//Used when checking the request sent from the reader
#define ISO15693_REQ_SUBCARRIER_SINGLE  0x00
#define ISO15693_REQ_SUBCARRIER_DUAL    0x01
#define ISO15693_REQ_DATARATE_LOW       0x00
#define ISO15693_REQ_DATARATE_HIGH      0x02


/* Codec Interface */
void ISO15693CodecInit(void);
void ISO15693CodecDeInit(void);
void ISO15693CodecTask(void);

/* Application Interface */
void ISO15693CodecStart(void);
void ISO15693CodecReset(void);



#endif  /* ISO15693_H_ */
