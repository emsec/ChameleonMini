/*
 * SniffISO15693.h
 *
 *  Created on: 05.11.2019
 *      Author: ceres-c
 */

#ifndef SNIFF_ISO15693_H_
#define SNIFF_ISO15693_H_

#include "Terminal/CommandLine.h"

#define ISO15693_APP_NO_RESPONSE        0x0000

#define SUBCARRIER_1                    32
#define SUBCARRIER_2                    28
#define SUBCARRIER_OFF                  0
#define SOF_PATTERN                     0x1D
#define EOF_PATTERN                     0xB8

//Used when checking the request sent from the reader
#define ISO15693_REQ_SUBCARRIER_SINGLE  0x00
#define ISO15693_REQ_SUBCARRIER_DUAL    0x01
#define ISO15693_REQ_DATARATE_LOW       0x00
#define ISO15693_REQ_DATARATE_HIGH      0x02

/* Codec Interface */
void SniffISO15693CodecInit(void);
void SniffISO15693CodecDeInit(void);
void SniffISO15693CodecTask(void);

/* Application Interface */
void SniffISO15693CodecStart(void);
void SniffISO15693CodecReset(void);

/* Internal functions */
INLINE void SNIFF_ISO15693_READER_EOC_VCD(void);
INLINE void CardSniffInit(void);


#endif  /* SNIFF_ISO15693_H_ */
