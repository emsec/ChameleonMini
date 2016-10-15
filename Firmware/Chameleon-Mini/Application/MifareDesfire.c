/*
 * MifareDesfire.c
 *
 *  Created on: 14.10.2016
 *      Author: dev_zzo
 */

#include "MifareDesfire.h"

#include "ISO14443-3A.h"
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
} DESFireCommandType;

typedef enum {
    /* Something went wrong or we've received a halt command */
    STATE_HALT,
    /* The card is powered up but not selected */
    STATE_IDLE,
    /* Entered on REQA or WUPA; anticollision is being performed */
    STATE_READY1,
    STATE_READY2,
    /* Entered when the card is selected and it's the first APDU to be handled */
    STATE_ACTIVE,
    /* Entered when the card is selected and is handling subsequent APDUs */
    STATE_ACTIVE_ISO14443,
    STATE_ACTIVE_ISO7816,
} StateType;

static StateType State;

/*
 * Application code
 */

void MifareDesfireAppInit(void)
{
    State = STATE_IDLE;
}

void MifareDesfireAppReset(void)
{
    State = STATE_IDLE;
}

void MifareDesfireAppTask(void)
{
    /* Empty */
}

static uint16_t MifareDesfireProcessNativeApdu(uint8_t* Buffer, uint16_t BitCountIn, bool LastBlock)
{
    switch (Buffer[0]) {

    /* ISO 14443-4 Commands (only one) */
    case ISO14443A_CMD_RATS:
        if (ISO14443ACheckCRCA(Buffer, ISO14443A_RATS_FRAME_SIZE)) {
            /* NOTE: ATS bytes are tailored to Chameleon implementation and differ from DESFire spec */
            /* NOTE: Some PCD implementations do a memcmp() over ATS bytes, which is completely wrong */
            Buffer[0] = 0x06; /* TL: 6 bytes */
            Buffer[1] = 0x75; /* T0: TA, TB, TC present; max accepted frame is 64 bytes */
            Buffer[2] = 0x00; /* TA: Only the lowest bit rate is supported */
            Buffer[3] = 0x81; /* TB: taken from the DESFire spec */
            Buffer[4] = 0x02; /* TC: taken from the DESFire spec */
            Buffer[5] = 0xEE; /* T1: dummy value for historical bytes */
            ISO14443AAppendCRCA(Buffer, ATS_FRAME_SIZE);
            return ATS_FRAME_SIZE;
        }
        goto checksum_fail;

    /* DESFire Commands */
    }

    return ISO14443A_APP_NO_RESPONSE;

length_fail:
    LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, BitCountIn);
    Buffer[0] = STATUS_LENGTH_ERROR;
    return STATUS_FRAME_SIZE;

checksum_fail:
    LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, BitCountIn);
    Buffer[0] = STATUS_INTEGRITY_ERROR;
    return STATUS_FRAME_SIZE;
}

static uint16_t MifareDesfireProcessIso14443Apdu(uint8_t* Buffer, uint16_t BitCountIn)
{
    /* ISO/IEC 14443-4; 7.1 Block format */
    uint8_t PCB = Buffer[0];

    /* Verify the block's length */
    if (BitCountIn < 3 * 8) {
        /* Huh? Broken frame? */
        /* TODO: LOG ME */
        return ISO14443A_APP_NO_RESPONSE;
    }
    BitCountIn -= 16;

    /* Verify the checksum; fail if doesn't match */
    if (!ISO14443ACheckCRCA(Buffer, BitCountIn / 8)) {
        LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, BitCountIn);
        /* ISO/IEC 14443-4, clause 7.5.5. The PICC does not attempt any error recovery. */
        return ISO14443A_APP_NO_RESPONSE;
    }

    switch (PCB & ISO14443_PCB_BLOCK_TYPE_MASK) {
    case ISO14443_PCB_I_BLOCK:
        if (PCB & (ISO14443_PCB_HAS_NAD_MASK | ISO14443_PCB_HAS_CID_MASK)) {
            /* Currently not supported => the frame is ignored */
            /* TODO: LOG ME */
            break;
        }
        if (PCB & ISO14443_PCB_I_BLOCK_CHAINING_MASK) {
            /* TODO: Verify block number */
            /* NOTE: Return value is ignored; the app doesn't reply to incomplete TX. */
            MifareDesfireProcessNativeApdu(Buffer + 1, BitCountIn - 8, false);
            Buffer[0] = ISO14443_PCB_R_BLOCK_STATIC | (PCB & ISO14443_PCB_BLOCK_NUMBER_MASK);
            ISO14443AAppendCRCA(Buffer, 1);
            return 3 * 8;
        }
        else {
            /* Handle the last block in the chain; return the app's response if any */
            return MifareDesfireProcessNativeApdu(Buffer + 1, BitCountIn - 8, true);
        }

    case ISO14443_PCB_R_BLOCK:
        break;
        
    case ISO14443_PCB_S_BLOCK:
        break;
    }

    return ISO14443A_APP_NO_RESPONSE;
}

static uint16_t MifareDesfireProcessIso7816Apdu(uint8_t* Buffer, uint16_t BitCountIn)
{
    return ISO14443A_APP_NO_RESPONSE;
}

uint16_t MifareDesfireAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
    switch (State) {
    case STATE_IDLE:
    case STATE_HALT:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE)) {
            /* We received a REQA or WUPA command, so wake up. */
            State = STATE_READY1;
            return BitCount;
        }
        break;

    case STATE_READY1:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE)) {
            State = STATE_READY1;
            return BitCount;
        }
        if (Buffer[0] == ISO14443A_CMD_SELECT_CL1) {
            /* Load UID CL1 and perform anticollision. */
            uint8_t UidCL1[ISO14443A_CL_UID_SIZE];

            UidCL1[0] = ISO14443A_UID0_CT;
            MemoryReadBlock(&UidCL1[1], MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
            if (ISO14443ASelect(Buffer, &BitCount, UidCL1, SAK_CL1_VALUE)) {
                /* CL1 stage has ended successfully */
                State = STATE_READY2;
            }

            return BitCount;
        }
        break;

    case STATE_READY2:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE)) {
            State = STATE_READY1;
            return BitCount;
        }
        if (Buffer[0] == ISO14443A_CMD_SELECT_CL2) {
            /* Load UID CL2 and perform anticollision */
            uint8_t UidCL2[ISO14443A_CL_UID_SIZE];

            MemoryReadBlock(UidCL2, MEM_UID_CL2_ADDRESS, MEM_UID_CL2_SIZE);
            if (ISO14443ASelect(Buffer, &BitCount, UidCL2, SAK_CL2_VALUE)) {
                /* CL2 stage has ended successfully. This means
                 * our complete UID has been sent to the reader. */
                State = STATE_ACTIVE;
            }

            return BitCount;
        }
        break;

    case STATE_ACTIVE:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE)) {
            State = STATE_READY1;
            return BitCount;
        }
        /* Check if this is an ISO 7816-4 APDU */
        if (Buffer[0] == 0x90 && Buffer[2] == 0x00 && Buffer[3] == 0x00) {
            /* ISO 7816-4 format */
            State = STATE_ACTIVE_ISO7816;
            return MifareDesfireProcessIso7816Apdu(Buffer, BitCount);
        }
        else {
            /* ISO 14443-4 format */
            State = STATE_ACTIVE_ISO14443;
            return MifareDesfireProcessIso14443Apdu(Buffer, BitCount);
        }
        break;

    case STATE_ACTIVE_ISO14443:
        return MifareDesfireProcessIso14443Apdu(Buffer, BitCount);

    case STATE_ACTIVE_ISO7816:
        return MifareDesfireProcessIso7816Apdu(Buffer, BitCount);
    }

    /* Unknown command. Enter halt state */
    State = STATE_HALT;
    return ISO14443A_APP_NO_RESPONSE;
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

