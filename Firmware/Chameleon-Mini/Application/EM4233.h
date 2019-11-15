/*
 * EM4233.h
 *
 *  Created on: 04.12.2018
 *      Author: ceres-c & MrMoDDoM
 */

#ifndef EM4233_H_
#define EM4233_H_

#include "Application.h"

#define EM4233_STD_UID_SIZE             ISO15693_GENERIC_UID_SIZE
#define EM4233_STD_MEM_SIZE             0xD0        // Bytes
#define EM4233_BYTES_PER_BLCK           0x04
#define EM4233_BLCKS_PER_PAGE           0x04
#define EM4233_NUMBER_OF_BLCKS          ( EM4233_STD_MEM_SIZE / EM4233_BYTES_PER_BLCK )
#define EM4233_NUMBER_OF_PAGES          ( EM4233_STD_MEM_SIZE / (EM4233_BYTES_PER_BLCK * EM4233_BLCKS_PER_PAGE) )

#define EM4233_IC_REFERENCE             0x02        // From EM4233SLIC datasheet and checked against real tags

#define EM4233_MEM_UID_ADDRESS          0xD0        // From 0x0100 to 0x0107 - UID
#define EM4233_MEM_AFI_ADDRESS          0xD8        // AFI byte address
#define EM4233_MEM_DSFID_ADDRESS        0xD9        // DSFID byte adress
#define EM4233_MEM_INF_ADDRESS          0xDC        // Some status bits

#define EM4233_MEM_LSM_ADDRESS          0xE0        // From 0xE0   to 0x0113 - Lock status masks
#define EM4233_MEM_PSW_ADDRESS          0x0114      // From 0x0114 to 0x0117 - 32 bit Password
#define EM4233_MEM_KEY_ADDRESS          0x0118      // From 0x0118 to 0x0123 - 96 bit Encryption Key

#define EM4233_SYSINFO_BYTE             0x0F        // == DSFID - AFI - VICC mem size - IC ref are present

/* Bit masks */
#define EM4233_MASK_READ_PROT           ( 1 << 2 )  // For lock status byte
#define EM4233_MASK_WRITE_PROT          ( 1 << 3 )
#define EM4233_MASK_AFI_STATUS          ( 1 << 0 )
#define EM4233_MASK_DSFID_STATUS        ( 1 << 1 )

/* Custom command code */
#define EM4233_CMD_SET_EAS              0xA2
#define EM4233_CMD_RST_EAS              0xA3
#define EM4233_CMD_LCK_EAS              0xA4
#define EM4233_CMD_ACT_EAS              0xA5
#define EM4233_CMD_PRT_EAS              0xA6
#define EM4233_CMD_WRT_EAS_ID           0xA7
#define EM4233_CMD_WRT_EAS_CFG          0xA8
#define EM4233_CMD_WRT_PSW              0xB4
#define EM4233_CMD_WRT_MEM_PAG          0xB6
#define EM4233_CMD_GET_BLKS_PRT_STS     0xB8
#define EM4233_CMD_DESTROY              0xB9
#define EM4233_CMD_ENABLE_PRCY          0xBA
#define EM4233_CMD_DISBLE_PRCY          0xBB
#define EM4233_CMD_FST_READ_BLKS        0xC3

/* Proprietary command code */
#define EM4233_CMD_AUTH1                0xE0
#define EM4233_CMD_AUTH2                0xE1
#define EM4233_CMD_GEN_READ             0xE2    // Implies some sort of singed CRC. Unknown at the moment
#define EM4233_CMD_GEN_WRITE            0xE3    // Same
#define EM4233_CMD_LOGIN                0xE4

/* Compile time switch */
/* EM4233_LOGIN_YES_CARD has to be uncommented if you want your emulated card
 * to accept any given password from the reader when a Login request (E4) is issued.
 * It is expecially useful when analyzing an unknown system and you want to fool a reader
 * into thiking you are using the original tag without actually knowing the password.
 */
#define EM4233_LOGIN_YES_CARD

void EM4233AppInit(void);
void EM4233AppReset(void);
void EM4233AppTask(void);
void EM4233AppTick(void);
uint16_t EM4233AppProcess(uint8_t *FrameBuf, uint16_t FrameBytes);
void EM4233GetUid(ConfigurationUidType Uid);
void EM4233SetUid(ConfigurationUidType Uid);
void EM4233FlipUid(ConfigurationUidType Uid);

#endif /* EM4233_H_ */
