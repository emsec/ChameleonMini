/*
 * MifareUltralight.c
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#include "MifareUltralight.h"
#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"


#define ATQA_VALUE              0x0044
#define SAK_CL1_VALUE           ISO14443A_SAK_INCOMPLETE
#define SAK_CL2_VALUE           ISO14443A_SAK_COMPLETE_NOT_COMPLIANT

#define ACK_VALUE               0x0A
#define ACK_FRAME_SIZE          4 /* Bits */
#define NAK_INVALID_ARG         0x00
#define NAK_CRC_ERROR           0x01
#define NAK_CTR_ERROR           0x04
#define NAK_EEPROM_ERROR        0x05
#define NAK_OTHER_ERROR         0x06
/* NOTE: the spec is not crystal clear which error is returned */
#define NAK_AUTH_REQUIRED       NAK_OTHER_ERROR
#define NAK_AUTH_FAILED         NAK_OTHER_ERROR
#define NAK_FRAME_SIZE          4

/* ISO commands */
#define CMD_HALT                0x50
/* EV0 commands */
#define CMD_READ                0x30
#define CMD_READ_FRAME_SIZE     2 /* without CRC bytes */
#define CMD_WRITE               0xA2
#define CMD_WRITE_FRAME_SIZE    6 /* without CRC bytes */
#define CMD_COMPAT_WRITE        0xA0
#define CMD_COMPAT_WRITE_FRAME_SIZE 2
/* EV1 commands */
#define CMD_GET_VERSION         0x60
#define CMD_FAST_READ           0x3A
#define CMD_READ_CNT            0x39
#define CMD_INCREMENT_CNT       0xA5
#define CMD_PWD_AUTH            0x1B
#define CMD_READ_SIG            0x3C
#define CMD_CHECK_TEARING_EVENT 0x3E
#define CMD_VCSL                0x4B

/* Tag memory layout; addresses and sizes in bytes */
#define UID_CL1_ADDRESS         0x00
#define UID_CL1_SIZE            3
#define UID_BCC1_ADDRESS        0x03
#define UID_CL2_ADDRESS         0x04
#define UID_CL2_SIZE            4
#define UID_BCC2_ADDRESS        0x08
#define LOCK_BYTES_1_ADDRESS    0x0A
#define LOCK_BYTES_2_ADDRESS    0x90
#define CONFIG_AREA_SIZE        0x10
#define CONF_AUTH0_OFFSET       0x03
#define CONF_ACCESS_OFFSET      0x04
#define CONF_VCTID_OFFSET       0x05
#define CONF_PASSWORD_OFFSET    0x08
#define CONF_PACK_OFFSET        0x0C

#define CONF_ACCESS_PROT        0x80
#define CONF_ACCESS_CNFLCK      0x40

#define CNT_MAX                 2
#define CNT_SIZE                4
#define CNT_MAX_VALUE           0x00FFFFFF

#define BYTES_PER_READ          16
#define PAGE_READ_MIN           0x00

#define BYTES_PER_WRITE         4
#define PAGE_WRITE_MIN          0x02

#define BYTES_PER_COMPAT_WRITE  16

#define VERSION_INFO_LENGTH     8
#define SIGNATURE_LENGTH        32

static enum {
    UL_EV0,
    UL_EV1,
} Flavor;

static enum {
    STATE_HALT,
    STATE_IDLE,
    STATE_READY1,
    STATE_READY2,
    STATE_ACTIVE,
    STATE_COMPAT_WRITE
} State;

static bool FromHalt = false;
static uint8_t PageCount;
static bool ArmedForCompatWrite;
static uint8_t CompatWritePageAddress;
static bool Authenticated;
static uint8_t FirstAuthenticatedPage;
static bool ReadAccessProtected;

static void AppInitCommon(void)
{
    State = STATE_IDLE;
    FromHalt = false;
    Authenticated = false;
    ArmedForCompatWrite = false;
}

void MifareUltralightAppInit(void)
{
    /* Set up the emulation flavor */
    Flavor = UL_EV0;
    /* EV0 cards have fixed size */
    PageCount = MIFARE_ULTRALIGHT_PAGES;
    /* Default values */
    FirstAuthenticatedPage = 0xFF;
    ReadAccessProtected = false;
    AppInitCommon();
}

static void AppInitEV1Common(void)
{
    uint8_t ConfigAreaAddress = PageCount * MIFARE_ULTRALIGHT_PAGE_SIZE - CONFIG_AREA_SIZE;
    uint8_t Access;

    /* Set up the emulation flavor */
    Flavor = UL_EV1;
    /* Fetch some of the configuration into RAM */
    MemoryReadBlock(&FirstAuthenticatedPage, ConfigAreaAddress + CONF_AUTH0_OFFSET, 1);
    MemoryReadBlock(&Access, ConfigAreaAddress + CONF_ACCESS_OFFSET, 1);
    ReadAccessProtected = !!(Access & CONF_ACCESS_PROT);
    AppInitCommon();
}

void MifareUltralightEV11AppInit(void)
{
    PageCount = MIFARE_ULTRALIGHT_EV11_PAGES;
    AppInitEV1Common();
}

void MifareUltralightEV12AppInit(void)
{
    PageCount = MIFARE_ULTRALIGHT_EV12_PAGES;
    AppInitEV1Common();
}

void MifareUltralightAppReset(void)
{
    State = STATE_IDLE;
}

void MifareUltralightAppTask(void)
{

}

static bool VerifyAuthentication(uint8_t PageAddress)
{
    /* No authentication for EV0 cards; always pass */
    if (Flavor < UL_EV1) {
        return true;
    }
    /* If authenticated, no verification needed */
    if (Authenticated) {
        return true;
    }
    /* Otherwise, verify the accessed page is below the limit */
    return PageAddress < FirstAuthenticatedPage;
}

static bool AuthCounterIncrement(void)
{
    /* Currently not implemented */
    return true;
}

static void AuthCounterReset(void)
{
    /* Currently not implemented */
}

/* Perform access verification and commit data if passed */
static uint8_t AppWritePage(uint8_t PageAddress, uint8_t* const Buffer)
{
    if (!ActiveConfiguration.ReadOnly) {
        MemoryWriteBlock(Buffer, PageAddress * MIFARE_ULTRALIGHT_PAGE_SIZE, MIFARE_ULTRALIGHT_PAGE_SIZE);
    } else {
        /* If the chameleon is in read only mode, it silently
        * ignores any attempt to write data. */
    }
    return 0;
}

/* Handles processing of MF commands */
static uint16_t AppProcess(uint8_t* const Buffer, uint16_t ByteCount)
{
    uint8_t Cmd = Buffer[0];

    /* Handle the compatibility write command */
    if (ArmedForCompatWrite) {
        ArmedForCompatWrite = false;
        AppWritePage(CompatWritePageAddress, &Buffer[2]);
        Buffer[0] = ACK_VALUE;
        return ACK_FRAME_SIZE;
    }

    /* Handle EV0 commands */
    switch (Cmd) {

        case CMD_READ: {
            uint8_t PageAddress = Buffer[1];
            uint8_t PageLimit;
            uint8_t Offset;
            /* For EV1+ cards, ensure the wraparound is at the first protected page */
            if (Flavor >= UL_EV1 && ReadAccessProtected && !Authenticated) {
                PageLimit = FirstAuthenticatedPage;
            } else {
                PageLimit = PageCount;
            }
            /* Validation */
            if (PageAddress >= PageLimit) {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }
            /* Read out, emulating the wraparound */
            for (Offset = 0; Offset < BYTES_PER_READ; Offset += 4) {
                MemoryReadBlock(&Buffer[Offset], PageAddress * MIFARE_ULTRALIGHT_PAGE_SIZE, MIFARE_ULTRALIGHT_PAGE_SIZE);
                PageAddress++;
                if (PageAddress == PageLimit) {
                    PageAddress = 0;
                }
            }
            ISO14443AAppendCRCA(Buffer, BYTES_PER_READ);
            return (BYTES_PER_READ + ISO14443A_CRCA_SIZE) * 8;
        }

        case CMD_WRITE: {
            /* This is a write command containing 4 bytes of data that
            * should be written to the given page address. */
            uint8_t PageAddress = Buffer[1];
            /* Validation */
            if ((PageAddress < PAGE_WRITE_MIN) || (PageAddress >= PageCount)) {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }
            if (!VerifyAuthentication(PageAddress)) {
                Buffer[0] = NAK_AUTH_REQUIRED;
                return NAK_FRAME_SIZE;
            }
            AppWritePage(PageAddress, &Buffer[2]);
            Buffer[0] = ACK_VALUE;
            return ACK_FRAME_SIZE;
        }

        case CMD_COMPAT_WRITE: {
            uint8_t PageAddress = Buffer[1];
            /* Validation */
            if ((PageAddress < PAGE_WRITE_MIN) || (PageAddress >= PageCount)) {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }
            if (!VerifyAuthentication(PageAddress)) {
                Buffer[0] = NAK_AUTH_REQUIRED;
                return NAK_FRAME_SIZE;
            }
            /* CRC check passed and page-address is within bounds.
            * Store address and proceed to receiving the data. */
            CompatWritePageAddress = PageAddress;
            ArmedForCompatWrite = true;
            Buffer[0] = ACK_VALUE;
            return ACK_FRAME_SIZE;
        }

        case CMD_HALT: {
            /* Halts the tag. According to the ISO14443, the second
            * byte is supposed to be 0. */
            if (Buffer[1] == 0) {
                /* According to ISO14443, we must not send anything
                * in order to acknowledge the HALT command. */
                State = STATE_HALT;
                return ISO14443A_APP_NO_RESPONSE;
            } else {
                Buffer[0] = NAK_INVALID_ARG;
                return NAK_FRAME_SIZE;
            }
        }
        default:
            break;
    }
    /* Handle EV1 commands */
    if (Flavor >= UL_EV1) {
        switch (Cmd) {

            case CMD_GET_VERSION: {
                /* Provide hardcoded version response */
                Buffer[0] = 0x00;
                Buffer[1] = 0x04;
                Buffer[2] = 0x03;
                Buffer[3] = 0x01; /**/
                Buffer[4] = 0x01;
                Buffer[5] = 0x00;
                Buffer[6] = PageCount == MIFARE_ULTRALIGHT_EV11_PAGES ? 0x0B : 0x0E;
                Buffer[7] = 0x03;
                ISO14443AAppendCRCA(Buffer, VERSION_INFO_LENGTH);
                return (VERSION_INFO_LENGTH + ISO14443A_CRCA_SIZE) * 8;
            }

            case CMD_FAST_READ: {
                uint8_t StartPageAddress = Buffer[1];
                uint8_t EndPageAddress = Buffer[2];
                /* Validation */
                if ((StartPageAddress > EndPageAddress) || (StartPageAddress >= PageCount) || (EndPageAddress >= PageCount)) {
                    Buffer[0] = NAK_INVALID_ARG;
                    return NAK_FRAME_SIZE;
                }
                /* Check authentication only if protection is read&write */
                if (ReadAccessProtected) {
                    if (!VerifyAuthentication(StartPageAddress) || !VerifyAuthentication(EndPageAddress)) {
                        Buffer[0] = NAK_AUTH_REQUIRED;
                        return NAK_FRAME_SIZE;
                    }
                }
                /* NOTE: With the current implementation, reading the password out is possible. */
                ByteCount = (EndPageAddress - StartPageAddress + 1) * MIFARE_ULTRALIGHT_PAGE_SIZE;
                MemoryReadBlock(Buffer, StartPageAddress * MIFARE_ULTRALIGHT_PAGE_SIZE, ByteCount);
                ISO14443AAppendCRCA(Buffer, ByteCount);
                return (ByteCount + ISO14443A_CRCA_SIZE) * 8;
            }

            case CMD_PWD_AUTH: {
                uint8_t ConfigAreaAddress = PageCount * MIFARE_ULTRALIGHT_PAGE_SIZE - CONFIG_AREA_SIZE;
                uint8_t Password[4];

                /* Verify value and increment authentication attempt counter */
                if (!AuthCounterIncrement()) {
                    /* Too many failed attempts */
                    Buffer[0] = NAK_AUTH_FAILED;
                    return NAK_FRAME_SIZE;
                }
                /* Read and compare the password */
                MemoryReadBlock(Password, ConfigAreaAddress + CONF_PASSWORD_OFFSET, 4);
                if (Password[0] != Buffer[1] || Password[1] != Buffer[2] || Password[2] != Buffer[3] || Password[3] != Buffer[4]) {
                    Buffer[0] = NAK_AUTH_FAILED;
                    return NAK_FRAME_SIZE;
                }
                /* Authenticate the user */
                AuthCounterReset();
                Authenticated = 1;
                /* Send the PACK value back */
                MemoryReadBlock(Buffer, ConfigAreaAddress + CONF_PACK_OFFSET, 2);
                ISO14443AAppendCRCA(Buffer, 2);
                return (2 + ISO14443A_CRCA_SIZE) * 8;
            }

            case CMD_READ_CNT: {
                uint8_t CounterId = Buffer[1];
                /* Validation */
                if (CounterId > CNT_MAX) {
                    Buffer[0] = NAK_INVALID_ARG;
                    return NAK_FRAME_SIZE;
                }
                /* Returned counter length is 3 bytes */
                MemoryReadBlock(Buffer, (PageCount + CounterId) * MIFARE_ULTRALIGHT_PAGE_SIZE, 3);
                ISO14443AAppendCRCA(Buffer, 3);
                return (3 + ISO14443A_CRCA_SIZE) * 8;
            }

            case CMD_INCREMENT_CNT: {
                uint8_t CounterId = Buffer[1];
                uint32_t Addend = (Buffer[0]) | (Buffer[1] << 8) | ((uint32_t)Buffer[2] << 16);
                uint32_t Counter;
                /* Validation */
                if (CounterId > CNT_MAX) {
                    Buffer[0] = NAK_INVALID_ARG;
                    return NAK_FRAME_SIZE;
                }
                /* Read the value out */
                MemoryReadBlock(&Counter, (PageCount + CounterId) * MIFARE_ULTRALIGHT_PAGE_SIZE, MIFARE_ULTRALIGHT_PAGE_SIZE);
                /* Add and check for overflow */
                Counter += Addend;
                if (Counter > CNT_MAX_VALUE) {
                    Buffer[0] = NAK_CTR_ERROR;
                    return NAK_FRAME_SIZE;
                }
                /* Update memory */
                MemoryWriteBlock(&Counter, (PageCount + CounterId) * MIFARE_ULTRALIGHT_PAGE_SIZE, MIFARE_ULTRALIGHT_PAGE_SIZE);
                Buffer[0] = ACK_VALUE;
                return ACK_FRAME_SIZE;
            }

            case CMD_READ_SIG:
                /* Hardcoded response */
                memset(Buffer, 0xCA, SIGNATURE_LENGTH);
                ISO14443AAppendCRCA(Buffer, SIGNATURE_LENGTH);
                return (SIGNATURE_LENGTH + ISO14443A_CRCA_SIZE) * 8;

            case CMD_CHECK_TEARING_EVENT:
                /* Hardcoded response */
                Buffer[0] = 0xBD;
                ISO14443AAppendCRCA(Buffer, 1);
                return (1 + ISO14443A_CRCA_SIZE) * 8;

            case CMD_VCSL: {
                uint8_t ConfigAreaAddress = PageCount * MIFARE_ULTRALIGHT_PAGE_SIZE - CONFIG_AREA_SIZE;
                /* Input is ignored completely */
                /* Read out the value */
                MemoryReadBlock(Buffer, ConfigAreaAddress + CONF_VCTID_OFFSET, 1);
                ISO14443AAppendCRCA(Buffer, 1);
                return (1 + ISO14443A_CRCA_SIZE) * 8;
            }

            default:
                break;
        }
    }
    /* Command not handled. Switch to idle. */
    State = STATE_IDLE;
    return ISO14443A_APP_NO_RESPONSE;
}

uint16_t MifareUltralightAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
    uint8_t Cmd = Buffer[0];
    uint16_t ByteCount;

    switch(State) {
    case STATE_IDLE:
    case STATE_HALT:
        FromHalt = State == STATE_HALT;
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
            /* We received a REQA or WUPA command, so wake up. */
            State = STATE_READY1;
            return BitCount;
        }
        break;

    case STATE_READY1:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
            State = FromHalt ? STATE_HALT : STATE_IDLE;
            return ISO14443A_APP_NO_RESPONSE;
        } else if (Cmd == ISO14443A_CMD_SELECT_CL1) {
            /* Load UID CL1 and perform anticollision. Since
            * MF Ultralight use a double-sized UID, the first byte
            * of CL1 has to be the cascade-tag byte. */
            uint8_t UidCL1[ISO14443A_CL_UID_SIZE] = { [0] = ISO14443A_UID0_CT };

            MemoryReadBlock(&UidCL1[1], UID_CL1_ADDRESS, UID_CL1_SIZE);

            if (ISO14443ASelect(Buffer, &BitCount, UidCL1, SAK_CL1_VALUE)) {
                /* CL1 stage has ended successfully */
                State = STATE_READY2;
            }

            return BitCount;
        } else {
            /* Unknown command. Enter halt state */
            State = STATE_IDLE;
        }
        break;

    case STATE_READY2:
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
            State = FromHalt ? STATE_HALT : STATE_IDLE;
            return ISO14443A_APP_NO_RESPONSE;
        } else if (Cmd == ISO14443A_CMD_SELECT_CL2) {
            /* Load UID CL2 and perform anticollision */
            uint8_t UidCL2[ISO14443A_CL_UID_SIZE];

            MemoryReadBlock(UidCL2, UID_CL2_ADDRESS, UID_CL2_SIZE);

            if (ISO14443ASelect(Buffer, &BitCount, UidCL2, SAK_CL2_VALUE)) {
                /* CL2 stage has ended successfully. This means
                * our complete UID has been sent to the reader. */
                State = STATE_ACTIVE;
            }

            return BitCount;
        } else {
            /* Unknown command. Enter halt state */
            State = STATE_IDLE;
        }
        break;

    case STATE_ACTIVE:
        /* Preserve incoming data length */
        ByteCount = (BitCount + 7) >> 3;
        if (ISO14443AWakeUp(Buffer, &BitCount, ATQA_VALUE, FromHalt)) {
            State = FromHalt ? STATE_HALT : STATE_IDLE;
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* At the very least, there should be 3 bytes in the buffer. */
        if (ByteCount < (1 + ISO14443A_CRCA_SIZE)) {
            State = STATE_IDLE;
            return ISO14443A_APP_NO_RESPONSE;
        }
        /* All commands here have CRCA appended; verify it right away */
        ByteCount -= 2;
        if (!ISO14443ACheckCRCA(Buffer, ByteCount)) {
            Buffer[0] = NAK_CRC_ERROR;
            return NAK_FRAME_SIZE;
        }
        return AppProcess(Buffer, ByteCount);

    default:
        /* Unknown state? Should never happen. */
        break;
    }

    /* No response has been sent, when we reach here */
    return ISO14443A_APP_NO_RESPONSE;
}

void MifareUltralightGetUid(ConfigurationUidType Uid)
{
    /* Read UID from memory */
    MemoryReadBlock(&Uid[0], UID_CL1_ADDRESS, UID_CL1_SIZE);
    MemoryReadBlock(&Uid[UID_CL1_SIZE], UID_CL2_ADDRESS, UID_CL2_SIZE);
}

void MifareUltralightSetUid(ConfigurationUidType Uid)
{
    /* Calculate check bytes and write everything into memory */
    uint8_t BCC1 = ISO14443A_UID0_CT ^ Uid[0] ^ Uid[1] ^ Uid[2];
    uint8_t BCC2 = Uid[3] ^ Uid[4] ^ Uid[5] ^ Uid[6];

    MemoryWriteBlock(&Uid[0], UID_CL1_ADDRESS, UID_CL1_SIZE);
    MemoryWriteBlock(&BCC1, UID_BCC1_ADDRESS, ISO14443A_CL_BCC_SIZE);
    MemoryWriteBlock(&Uid[UID_CL1_SIZE], UID_CL2_ADDRESS, UID_CL2_SIZE);
    MemoryWriteBlock(&BCC2, UID_BCC2_ADDRESS, ISO14443A_CL_BCC_SIZE);
}

