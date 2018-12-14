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
#define EM4233_STD_MEM_SIZE             256     // Bytes
#define EM4233_BYTES_PER_BLCK           0x4
#define EM4233_BLCKS_PER_PAGE           0x4
#define EM4233_NUMBER_OF_BLCKS          ( EM4233_STD_MEM_SIZE / EM4233_BYTES_PER_BLCK )
#define EM4233_NUMBER_OF_PAGES          ( EM4233_STD_MEM_SIZE / (EM4233_BYTES_PER_BLCK * EM4233_BLCKS_PER_PAGE) )

#define EM4233_IC_REFERENCE             0x02

#define EM4233_USR_MEM_SIZE             64          // Bytes, guessed, not described anywere
#define EM4233_MEM_UID_ADDRESS          0x0100      // From 0x0100 to 0x0107 - UID
#define EM4233_MEM_AFI_ADDRESS          0x0108      // AFI byte address
#define EM4233_MEM_DSFID_ADDRESS        0x0109      // DSFID byte adress
#define EM4233_MEM_INF_ADDRESS          0x010C      // Some status bits

#define EM4233_MEM_LSM_ADDRESS          0x0110      // From 0x0108 to 0x0157 - Lock status masks
#define EM4233_MEM_PSW_ADDRESS          0x0150      // From 0x0118 to 0x011F - Password
#define EM4233_MEM_KEY_ADDRESS          0x0120      // From 0x0120 to 0x0127 - Encryption Key

#define EM4233_SYSINFO_BYTE             0x0F        // DSFID - AFI - VICC mem size - IC ref are present

/* Bit masks */
#define EM4233_MASK_READ_PROT           ( 1 << 2 )  // For lock status byte
#define EM4233_MASK_WRITE_PROT          ( 1 << 3 )
#define EM4233_MASK_AFI_STATUS          ( 1 << 0 ) 
#define EM4233_MASK_DSFID_STATUS        ( 1 << 1 ) 

#define EM4233_TOT_MEM_SIZE             ( EM4233_STD_MEM_SIZE + EM4233_USR_MEM_SIZE )

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
#define EM4233_CMD_LOGIN                0xE4


void EM4233AppInit(void);
void EM4233AppReset(void);
void EM4233AppTask(void);
void EM4233AppTick(void);
uint16_t EM4233AppProcess(uint8_t* FrameBuf, uint16_t FrameBytes);
void EM4233GetUid(ConfigurationUidType Uid);
void EM4233SetUid(ConfigurationUidType Uid);
void EM4233FlipUid(ConfigurationUidType Uid);

#endif /* EM4233_H_ */
