/*
 * ISO14443-4.h
 *
 *  Created on: 15.10.2016
 *      Author: dev_zzo
 */

#ifndef ISO14443_4_H_
#define ISO14443_4_H_

/* General structure of a ISO 14443-4 block:
 * PCB (protocol control byte)
 * CID (card identifier byte; presence controlled by PCB)
 * NAD (node address byte; presence controlled by PCB)
 * Payload (arbitrary bytes)
 * CRC-16
 */

#define ISO14443_PCB_BLOCK_TYPE_MASK 0xC0
#define ISO14443_PCB_I_BLOCK 0x00
#define ISO14443_PCB_R_BLOCK 0x80
#define ISO14443_PCB_S_BLOCK 0xC0

#define ISO14443_PCB_I_BLOCK_STATIC 0x02
#define ISO14443_PCB_R_BLOCK_STATIC 0xA2
#define ISO14443_PCB_S_BLOCK_STATIC 0xC2

#define ISO14443_PCB_BLOCK_NUMBER_MASK 0x01
#define ISO14443_PCB_HAS_NAD_MASK 0x04
#define ISO14443_PCB_HAS_CID_MASK 0x08
#define ISO14443_PCB_I_BLOCK_CHAINING_MASK 0x10
#define ISO14443_PCB_R_BLOCK_ACKNAK_MASK 0x10

#endif /* ISO14443_4_H_ */
