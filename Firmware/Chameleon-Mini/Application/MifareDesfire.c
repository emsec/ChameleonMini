/*
 * MifareDesfire.c
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#ifdef CONFIG_MF_DESFIRE_SUPPORT

#include "MifareDesfire.h"

#include "ISO14443-3A.h"
#include "ISO14443-4.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"
#include "../Random.h"

/* Anticollision parameters */
#define ATQA_VALUE              0x0344
#define SAK_CL1_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT | ISO14443A_SAK_INCOMPLETE)
#define SAK_CL2_VALUE           (ISO14443A_SAK_COMPLETE_COMPLIANT)

/* UID location in the card memory; sizes are in bytes */
#define MEM_UID_CL1_ADDRESS         0x00
#define MEM_UID_CL1_SIZE            3
#define MEM_UID_BCC1_ADDRESS        0x03
#define MEM_UID_CL2_ADDRESS         0x04
#define MEM_UID_CL2_SIZE            4
#define MEM_UID_BCC2_ADDRESS        0x08

#define STATUS_FRAME_SIZE           (1 * 8) /* Bits */
#define ATS_FRAME_SIZE              6 /* Bytes */

#define ATS_TL_BYTE 0x06 /* TL: ATS length, 6 bytes */
#define ATS_T0_BYTE 0x75 /* T0: TA, TB, TC present; max accepted frame is 64 bytes */
#define ATS_TA_BYTE 0x00 /* TA: Only the lowest bit rate is supported */
#define ATS_TB_BYTE 0x81 /* TB: taken from the DESFire spec */
#define ATS_TC_BYTE 0x02 /* TC: taken from the DESFire spec */

/* Defines for GetVersion */
#define VENDOR_ID_PHILIPS_NXP           0x04
#define DESFIRE_TYPE_MF3ICD40           0x01
#define DESFIRE_SUBTYPE_MF3ICD40        0x01
#define DESFIRE_HW_MAJOR_VERSION_NO     0x01
#define DESFIRE_HW_MINOR_VERSION_NO     0x01
#define DESFIRE_HW_PROTOCOL_TYPE        0x05
#define DESFIRE_SW_MAJOR_VERSION_NO     0x01
#define DESFIRE_SW_MINOR_VERSION_NO     0x01
#define DESFIRE_SW_PROTOCOL_TYPE        0x05
#define DESFIRE_STORAGE_SIZE            0x18 /* = 4k */

/* Based on section 3.4 */
typedef enum {
    STATUS_OPERATION_OK = 0x00,

    STATUS_NO_CHANGES = 0x0C,
    STATUS_OUT_OF_EEPROM_ERROR = 0x0E,

    STATUS_ILLEGAL_COMMAND_CODE = 0x1C,
    STATUS_INTEGRITY_ERROR = 0x1E,

    STATUS_NO_SUCH_KEY = 0x40,

    STATUS_LENGTH_ERROR = 0x7E,

    STATUS_PERMISSION_DENIED = 0x9D,
    STATUS_PARAMETER_ERROR = 0x9E,

    STATUS_APP_NOT_FOUND = 0xA0,
    STATUS_APP_INTEGRITY_ERROR = 0xA1,
    STATUS_AUTHENTICATION_ERROR = 0xAE,
    STATUS_ADDITIONAL_FRAME = 0xAF,
    STATUS_BOUNDARY_ERROR = 0xBE,
    STATUS_COMMAND_ABORTED = 0xCA,
    STATUS_COUNT_ERROR = 0xCE,
    STATUS_DUPLICATE_ERROR = 0xDE,
    STATUS_EEPROM_ERROR = 0xEE,

    STATUS_FILE_NOT_FOUND = 0xF0,
} DesfireStatusCodeType;

typedef enum {
    CMD_AUTHENTICATE = 0x0A,
    CMD_CHANGE_KEY_SETTINGS = 0x54,
    CMD_GET_KEY_SETTINGS = 0x45,
    CMD_CHANGE_KEY = 0xC4,
    CMD_GET_KEY_VERSION = 0x64,

    CMD_GET_VERSION = 0x60,

    CMD_CREATE_APPLICATION = 0xCA,
    CMD_DELETE_APPLICATION = 0xDA,
    CMD_GET_APPLICATION_IDS = 0x6A,
    CMD_SELECT_APPLICATION = 0x5A,

    CMD_FORMAT_PICC = 0xFC,

    CMD_GET_FILE_IDS = 0x6F,
    CMD_GET_FILE_SETTINGS = 0xF5,
    CMD_CHANGE_FILE_SETTINGS = 0x5F,
    CMD_CREATE_STANDARD_DATA_FILE = 0xCD,
    CMD_CREATE_BACKUP_DATA_FILE = 0xCB,
    CMD_CREATE_VALUE_FILE = 0xCC,
    CMD_CREATE_LINEAR_RECORD_FILE = 0xC1,
    CMD_CREATE_CYCLIC_RECORD_FILE = 0xC0,
    CMD_DELETE_FILE = 0xDF,
    CMD_READ_DATA = 0xBD,
    CMD_WRITE_DATA = 0x3D,
    CMD_GET_VALUE = 0x6C,
    CMD_CREDIT = 0x0C,
    CMD_DEBIT = 0xDC,
    CMD_LIMITED_CREDIT = 0x1C,
    CMD_WRITE_RECORD = 0x3B,
    CMD_READ_RECORDS = 0xBB,
    CMD_CLEAR_RECORD_FILE = 0xEB,
    CMD_COMMIT_TRANSACTION = 0xC7,
    CMD_ABORT_TRANSACTION = 0xA7,
} DesfireCommandType;

/*
 * DESFire application code
 */

void ISO144434Init(void);
void ISO144433AInit(void);

typedef enum {
    DESFIRE_IDLE,
    /* Handling GetVersion's multiple frames */
    DESFIRE_GET_VERSION2,
    DESFIRE_GET_VERSION3,
} DesfireStateType;

static DesfireStateType DesfireState;

void MifareDesfireAppInit(void)
{
    ISO144433AInit();
    ISO144434Init();
    DesfireState = DESFIRE_IDLE;
}

void MifareDesfireAppReset(void)
{
    MifareDesfireAppInit();
}

void MifareDesfireAppTask(void)
{
    /* Empty */
}

static bool MifareDesfireReceiveBlock(uint8_t* Buffer, uint16_t* ByteCount, bool LastBlock)
{
    switch (DESFireState) {
    case DESFIRE_IDLE:
        /* This is the first command frame. */
        switch (Buffer[0]) {
        case CMD_AUTHENTICATE:
            break;

        case CMD_GET_VERSION:
            Buffer[0] = STATUS_ADDITIONAL_FRAME;
            Buffer[1] = VENDOR_ID_PHILIPS_NXP;
            Buffer[2] = DESFIRE_TYPE_MF3ICD40;
            Buffer[3] = DESFIRE_SUBTYPE_MF3ICD40;
            Buffer[4] = DESFIRE_HW_MAJOR_VERSION_NO;
            Buffer[5] = DESFIRE_HW_MINOR_VERSION_NO;
            Buffer[6] = DESFIRE_STORAGE_SIZE;
            Buffer[7] = DESFIRE_HW_PROTOCOL_TYPE;
            *ByteCount = 8;
            DesfireState = DESFIRE_GET_VERSION2;
            break;
        }
        break;

    case DESFIRE_GET_VERSION2:
        Buffer[0] = STATUS_ADDITIONAL_FRAME;
        Buffer[1] = VENDOR_ID_PHILIPS_NXP;
        Buffer[2] = DESFIRE_TYPE_MF3ICD40;
        Buffer[3] = DESFIRE_SUBTYPE_MF3ICD40;
        Buffer[4] = DESFIRE_SW_MAJOR_VERSION_NO;
        Buffer[5] = DESFIRE_SW_MINOR_VERSION_NO;
        Buffer[6] = DESFIRE_STORAGE_SIZE;
        Buffer[7] = DESFIRE_SW_PROTOCOL_TYPE;
        *ByteCount = 8;
        DesfireState = DESFIRE_GET_VERSION3;
        break;

    case DESFIRE_GET_VERSION3:
        Buffer[0] = STATUS_OPERATION_OK;
        /* UID */
        /* Batch number */
        Buffer[8] = 0;
        Buffer[9] = 0;
        Buffer[10] = 0;
        Buffer[11] = 0;
        Buffer[12] = 0;
        /* Calendar week and year */
        Buffer[13] = 52;
        Buffer[14] = 16;
        break;
    }

    return false;
}

static bool MifareDesfireSendBlock(uint8_t* Buffer, uint16_t* ByteCount, bool Retransmit)
{
    return false;
}

/*
 * ISO/IEC 14443-4 implementation
 */

void ISO144433AHalt(void);

typedef enum {
    ISO14443_4_STATE_EXPECT_RATS,
    ISO14443_4_STATE_ACTIVE,
} Iso144434StateType;

static Iso144434StateType Iso144434State;
static uint8_t Iso144434BlockNumber;

void ISO144434Init(void)
{
    Iso144434State = ISO14443_4_STATE_EXPECT_RATS;
    Iso144434BlockNumber = 1;
}

uint16_t ISO144434ProcessBlock(uint8_t* Buffer, uint16_t ByteCount)
{
    uint8_t PCB = Buffer[0];
    uint8_t MyBlockNumber = Iso144434BlockNumber;

    /* Verify the block's length */
    if (ByteCount < 3) {
        /* Huh? Broken frame? */
        /* TODO: LOG ME */
        return ISO14443A_APP_NO_RESPONSE;
    }
    ByteCount -= 2;

    /* Verify the checksum; fail if doesn't match */
    if (!ISO14443ACheckCRCA(Buffer, ByteCount)) {
        LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, ByteCount);
        /* ISO/IEC 14443-4, clause 7.5.5. The PICC does not attempt any error recovery. */
        return ISO14443A_APP_NO_RESPONSE;
    }

    switch (Iso144434State) {
    case ISO14443_4_STATE_EXPECT_RATS:
        /* See: ISO/IEC 14443-4, clause 5.6.1.2 */
        if (Buffer[0] != ISO14443A_CMD_RATS) {
            /* Ignore blocks other than RATS and HLTA */
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Process RATS.
         * NOTE: ATS bytes are tailored to Chameleon implementation and differ from DESFire spec.
         * NOTE: Some PCD implementations do a memcmp() over ATS bytes, which is completely wrong.
         */
        Buffer[0] = ATS_TL_BYTE;
        Buffer[1] = ATS_T0_BYTE;
        Buffer[2] = ATS_TA_BYTE;
        Buffer[3] = ATS_TB_BYTE;
        Buffer[4] = ATS_TC_BYTE;
        Buffer[5] = 0x00; /* T1: dummy value for historical bytes */
        Iso144434State = ISO14443_4_STATE_ACTIVE;
        ByteCount = ATS_FRAME_SIZE;
        break;

    case ISO14443_4_STATE_ACTIVE:
        /* See: ISO/IEC 14443-4; 7.1 Block format */
        switch (PCB & ISO14443_PCB_BLOCK_TYPE_MASK) {
        case ISO14443_PCB_I_BLOCK:
            if (PCB & (ISO14443_PCB_HAS_NAD_MASK | ISO14443_PCB_HAS_CID_MASK)) {
                /* Currently not supported => the frame is ignored */
                /* TODO: LOG ME */
                return ISO14443A_APP_NO_RESPONSE;
            }
            /* 7.5.3.2, rule D: toggle on each I-block */
            Iso144434BlockNumber = MyBlockNumber = !MyBlockNumber;

            ByteCount -= 1;
            if (PCB & ISO14443_PCB_I_BLOCK_CHAINING_MASK) {
                /* NOTE: Return value is ignored; the app doesn't reply to incomplete TX. */
                MifareDesfireReceiveBlock(Buffer + 1, &ByteCount, false);
                PCB = ISO14443_PCB_R_BLOCK_STATIC | (PCB & ISO14443_PCB_BLOCK_NUMBER_MASK);
                ByteCount = ISO14443_R_BLOCK_SIZE;
            }
            else {
                /* 7.5.4.3, rule 10: last block is acknowledged by an I-block */
                PCB = ISO14443_PCB_I_BLOCK_STATIC | MyBlockNumber;
                if (MifareDesfireReceiveBlock(Buffer + 1, &ByteCount, true)) {
                    PCB |= ISO14443_PCB_I_BLOCK_CHAINING_MASK;
                }
                /* ByteCount is set by the application */
            }
            Buffer[0] = PCB;
            break;

        case ISO14443_PCB_R_BLOCK:
            if ((PCB & ISO14443_PCB_BLOCK_NUMBER_MASK) == MyBlockNumber) {
                /* 7.5.4.3, rule 11: Unconditional retransmission */
                PCB = ISO14443_PCB_I_BLOCK_STATIC | MyBlockNumber;
                if (MifareDesfireSendBlock(Buffer + 1, &ByteCount, true)) {
                    PCB |= ISO14443_PCB_I_BLOCK_CHAINING_MASK;
                }
            }
            else {
                if (PCB & ISO14443_PCB_R_BLOCK_ACKNAK_MASK) {
                    /* Handle NAK */
                    /* 7.5.4.3, rule 12: send ACK */
                    PCB = ISO14443_PCB_R_BLOCK_STATIC | MyBlockNumber;
                    ByteCount = ISO14443_R_BLOCK_SIZE;
                }
                else {
                    /* Handle ACK */
                    /* 7.5.3.2, rule E: toggle on R(ACK) */
                    Iso144434BlockNumber = MyBlockNumber = !MyBlockNumber;
                    /* 7.5.4.3, rule 13: send the next block */
                    PCB = ISO14443_PCB_I_BLOCK_STATIC | MyBlockNumber;
                    if (MifareDesfireSendBlock(Buffer + 1, &ByteCount, false)) {
                        PCB |= ISO14443_PCB_I_BLOCK_CHAINING_MASK;
                    }
                }
            }
            Buffer[0] = PCB;
            break;

        case ISO14443_PCB_S_BLOCK:
            if (PCB == ISO14443_PCB_S_DESELECT) {
                ISO144433AHalt();
            }
            return ISO14443A_APP_NO_RESPONSE;
        }
        break;
    }

    ISO14443AAppendCRCA(Buffer, ByteCount);
    return ByteCount + ISO14443A_CRCA_SIZE;
}

/*
 * ISO/IEC 14443-3A implementation
 */

typedef enum {
    /* Something went wrong or we've received a halt command */
    ISO14443_3A_STATE_HALT,
    /* The card is powered up but not selected */
    ISO14443_3A_STATE_IDLE,
    /* Entered on REQA or WUPA; anticollision is being performed */
    ISO14443_3A_STATE_READY1,
    ISO14443_3A_STATE_READY2,
    /* Entered when the card has been selected */
    ISO14443_3A_STATE_ACTIVE,
} Iso144433AStateType;

static Iso144433AStateType Iso144433AState;
static Iso144433AStateType Iso144433AIdleState;

void ISO144433AInit(void)
{
    Iso144433AState = ISO14443_3A_STATE_IDLE;
}

void ISO144433AHalt(void)
{
    Iso144433AState = ISO14443_3A_STATE_HALT;
}

static bool ISO144433AIsHalt(const uint8_t* Buffer, uint16_t BitCount)
{
    return BitCount == ISO14443A_HLTA_FRAME_SIZE + ISO14443A_CRCA_SIZE * 8
        && Buffer[0] == ISO14443A_CMD_HLTA
        && Buffer[1] == 0x00
        && ISO14443ACheckCRCA(Buffer, ISO14443A_HLTA_FRAME_SIZE / 8);
}

uint16_t ISO144433APiccProcess(uint8_t* Buffer, uint16_t BitCount)
{
    uint8_t Cmd = Buffer[0];

    /* This implements ISO 14443-3A state machine */
    /* See: ISO/IEC 14443-3, clause 6.2 */
    switch (Iso144433AState) {
    case ISO14443_3A_STATE_IDLE:
        if (Cmd != ISO14443A_CMD_REQA)
            break;
        /* Fall-through */

    case ISO14443_3A_STATE_HALT:
        if (Cmd != ISO14443A_CMD_WUPA)
            break;
        Iso144433AIdleState = Iso144433AState;
        Iso144433AState = ISO14443_3A_STATE_READY1;
        Buffer[0] = (ATQA_VALUE >> 0) & 0x00FF;
        Buffer[1] = (ATQA_VALUE >> 8) & 0x00FF;
        return ISO14443A_ATQA_FRAME_SIZE;

    case ISO14443_3A_STATE_READY1:
        if (Cmd == ISO14443A_CMD_SELECT_CL1) {
            /* Load UID CL1 and perform anticollision. */
            ConfigurationUidType Uid;

            ApplicationGetUid(Uid);
            if (ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
                Uid[3] = Uid[2];
                Uid[2] = Uid[1];
                Uid[1] = Uid[0];
                Uid[0] = ISO14443A_UID0_CT;
            }
            if (ISO14443ASelect(Buffer, &BitCount, Uid, SAK_CL1_VALUE)) {
                /* CL1 stage has ended successfully */
                Iso144433AState = ISO14443_3A_STATE_READY2;
            }

            return BitCount;
        }
        break;

    case ISO14443_3A_STATE_READY2:
        if (Cmd == ISO14443A_CMD_SELECT_CL2 && ActiveConfiguration.UidSize >= ISO14443A_UID_SIZE_DOUBLE) {
            /* Load UID CL2 and perform anticollision */
            ConfigurationUidType Uid;

            ApplicationGetUid(Uid);
            if (ISO14443ASelect(Buffer, &BitCount, &Uid[3], SAK_CL2_VALUE)) {
                /* CL2 stage has ended successfully. This means
                 * our complete UID has been sent to the reader. */
                Iso144433AState = ISO14443_3A_STATE_ACTIVE;
            }

            return BitCount;
        }
        break;

    case ISO14443_3A_STATE_ACTIVE:
        /* Recognise the HLTA command */
        if (ISO144433AIsHalt(Buffer, BitCount)) {
            LogEntry(LOG_INFO_APP_CMD_HALT, NULL, 0);
            Iso144433AState = ISO14443_3A_STATE_HALT;
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* Forward to ISO/IEC 14443-4 processing code */
        return ISO144434ProcessBlock(Buffer, BitCount / 8) * 8;
    }

    /* Unknown command. Reset back to idle/halt state. */
    Iso144433AState = Iso144433AIdleState;
    return ISO14443A_APP_NO_RESPONSE;
}

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
    return ISO144433APiccProcess(Buffer, BitCount);
}

void MifareDesfireGetUid(ConfigurationUidType Uid)
{
    MemoryReadBlock(&Uid[0], MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE-1);
    MemoryReadBlock(&Uid[3], MEM_UID_CL2_ADDRESS, MEM_UID_CL2_SIZE);
}

void MifareDesfireSetUid(ConfigurationUidType Uid)
{
    MemoryWriteBlock(Uid, MEM_UID_CL1_ADDRESS, ActiveConfiguration.UidSize);
}

#endif /* CONFIG_MF_DESFIRE_SUPPORT */
