#if defined(CONFIG_ISO14443A_READER_SUPPORT) || defined(CONFIG_ISO14443A_SNIFF_SUPPORT)

#include "Reader14443A.h"
#include "LEDHook.h"
#include "Application.h"
#include "ISO14443-3A.h"
#include "../Codec/Reader14443-2A.h"
#include "Crypto1.h"
#include "../System.h"

#include "../Terminal/Terminal.h"

#define CHECK_BCC(B) ((B[0] ^ B[1] ^ B[2] ^ B[3]) == B[4])
#define IS_CASCADE_BIT_SET(buf) (buf[0] & 0x04)
#define IS_ISO14443A_4_COMPLIANT(buf) (buf[0] & 0x20)

#define TRYCOUNT_MAX    16

#define FLAGS_MASK		0x03
#define FLAGS_PARITY_OK	0x01
#define FLAGS_NO_DATA	0x02

// TODO replace remaining magic numbers

uint8_t ReaderSendBuffer[CODEC_BUFFER_SIZE];
uint16_t ReaderSendBitCount;

static bool Selected = false;
Reader14443Command Reader14443CurrentCommand = Reader14443_Do_Nothing;

static enum {
    STATE_IDLE,
    STATE_HALT,
    STATE_READY,
    STATE_ACTIVE_CL1, // must be ordered sequentially
    STATE_ACTIVE_CL2,
    STATE_ACTIVE_CL3,
    STATE_SAK_CL1, // must be ordered sequentially
    STATE_SAK_CL2,
    STATE_SAK_CL3,
    STATE_ATS,
    STATE_DESELECT,
    STATE_DESFIRE_INFO,
    STATE_UL_C_AUTH,
    STATE_UL_EV1_GETVERSION,
    STATE_END
} ReaderState = STATE_IDLE;

static struct {
    uint16_t ATQA;
    uint8_t SAK;
    uint8_t UID[10];
    enum {
        UIDSize_No_UID = 0,
        UIDSize_Single = 4,
        UIDSize_Double = 7,
        UIDSize_Triple = 10
    } UIDSize;
} CardCharacteristics = {0};

typedef enum {
    CardType_NXP_MIFARE_Mini = 0, // do NOT assign another CardType item with a specific value since there are loops over this type
    CardType_NXP_MIFARE_Classic_1k,
    CardType_NXP_MIFARE_Classic_4k,
    CardType_NXP_MIFARE_Ultralight,
//	CardType_NXP_MIFARE_Ultralight_C,
//	CardType_NXP_MIFARE_Ultralight_EV1,
    CardType_NXP_MIFARE_DESFire,
    CardType_NXP_MIFARE_DESFire_EV1,
    CardType_IBM_JCOP31,
    CardType_IBM_JCOP31_v241,
    CardType_IBM_JCOP41_v22,
    CardType_IBM_JCOP41_v231,
    CardType_Infineon_MIFARE_Classic_1k,
    CardType_Gemplus_MPCOS,
    CardType_Innovision_Jewel,
    CardType_Nokia_MIFARE_Classic_4k_emulated_6212,
    CardType_Nokia_MIFARE_Classic_4k_emulated_6131
} CardType;

typedef struct {
    uint16_t ATQA;
    bool ATQARelevant;

    uint8_t SAK;
    bool SAKRelevant;

    uint8_t ATS[16];
    uint8_t ATSSize;
    bool ATSRelevant;

    char Manufacturer[16];
    char Type[64];
} CardIdentificationType;

static const CardIdentificationType PROGMEM CardIdentificationList[] = {
    [CardType_NXP_MIFARE_Mini] 				= { .ATQA = 0x0004, .ATQARelevant = true, .SAK = 0x09, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "NXP", .Type = "MIFARE Mini" },
    [CardType_NXP_MIFARE_Classic_1k] 		= { .ATQA = 0x0004, .ATQARelevant = true, .SAK = 0x08, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "NXP", .Type = "MIFARE Classic 1k" },
    [CardType_NXP_MIFARE_Classic_4k] 		= { .ATQA = 0x0002, .ATQARelevant = true, .SAK = 0x18, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "NXP", .Type = "MIFARE Classic 4k" },
    [CardType_NXP_MIFARE_Ultralight]        = { .ATQA = 0x0044, .ATQARelevant = true, .SAK = 0x00, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "NXP", .Type = "MIFARE Ultralight" },
    //[CardType_NXP_MIFARE_Ultralight_C]      = { .ATQA=0x0044, .ATQARelevant=true, .SAK=0x00, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="NXP", .Type="MIFARE Ultralight C" },
    //[CardType_NXP_MIFARE_Ultralight_EV1]    = { .ATQA=0x0044, .ATQARelevant=true, .SAK=0x00, .SAKRelevant=false, .ATSRelevant=false, .Manufacturer="NXP", .Type="MIFARE Ultralight EV1" },
    // for the following two, setting ATSRelevant to true would cause checking the ATS value, but the NXP paper for distinguishing cards does not recommend this
    [CardType_NXP_MIFARE_DESFire] 			= { .ATQA = 0x0344, .ATQARelevant = true, .SAK = 0x20, .SAKRelevant = true, .ATSRelevant = false, .ATSSize = 5, .ATS = {0x75, 0x77, 0x81, 0x02, 0x80}, .Manufacturer = "NXP", .Type = "MIFARE DESFire" },
    [CardType_NXP_MIFARE_DESFire_EV1] 		= { .ATQA = 0x0344, .ATQARelevant = true, .SAK = 0x20, .SAKRelevant = true, .ATSRelevant = false, .ATSSize = 5, .ATS = {0x75, 0x77, 0x81, 0x02, 0x80}, .Manufacturer = "NXP", .Type = "MIFARE DESFire EV1" },
    [CardType_IBM_JCOP31] 					= { .ATQA = 0x0304, .ATQARelevant = true, .SAK = 0x28, .SAKRelevant = true, .ATSRelevant = true, .ATSSize = 9, .ATS = {0x38, 0x77, 0xb1, 0x4a, 0x43, 0x4f, 0x50, 0x33, 0x31}, .Manufacturer = "IBM", .Type = "JCOP31" },
    [CardType_IBM_JCOP31_v241] 				= { .ATQA = 0x0048, .ATQARelevant = true, .SAK = 0x20, .SAKRelevant = true, .ATSRelevant = true, .ATSSize = 12, .ATS = {0x78, 0x77, 0xb1, 0x02, 0x4a, 0x43, 0x4f, 0x50, 0x76, 0x32, 0x34, 0x31}, .Manufacturer = "IBM", .Type = "JCOP31 v2.4.1" },
    [CardType_IBM_JCOP41_v22] 				= { .ATQA = 0x0048, .ATQARelevant = true, .SAK = 0x20, .SAKRelevant = true, .ATSRelevant = true, .ATSSize = 12, .ATS = {0x38, 0x33, 0xb1, 0x4a, 0x43, 0x4f, 0x50, 0x34, 0x31, 0x56, 0x32, 0x32}, .Manufacturer = "IBM", .Type = "JCOP41 v2.2" },
    [CardType_IBM_JCOP41_v231] 				= { .ATQA = 0x0004, .ATQARelevant = true, .SAK = 0x28, .SAKRelevant = true, .ATSRelevant = true, .ATSSize = 13, .ATS = {0x38, 0x33, 0xb1, 0x4a, 0x43, 0x4f, 0x50, 0x34, 0x31, 0x56, 0x32, 0x33, 0x31}, .Manufacturer = "IBM", .Type = "JCOP41 v2.3.1" },
    [CardType_Infineon_MIFARE_Classic_1k] 	= { .ATQA = 0x0004, .ATQARelevant = true, .SAK = 0x88, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "Infineon", .Type = "MIFARE Classic 1k" },
    [CardType_Gemplus_MPCOS] 				= { .ATQA = 0x0002, .ATQARelevant = true, .SAK = 0x98, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "Gemplus", .Type = "MPCOS" },
    [CardType_Innovision_Jewel] 			= { .ATQA = 0x0C00, .ATQARelevant = true, .SAKRelevant = false, .ATSRelevant = false, .Manufacturer = "Innovision R&T", .Type = "Jewel" },
    [CardType_Nokia_MIFARE_Classic_4k_emulated_6212] = { .ATQA = 0x0002, .ATQARelevant = true, .SAK = 0x38, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "Nokia", .Type = "MIFARE Classic 4k - emulated (6212 Classic)" },
    [CardType_Nokia_MIFARE_Classic_4k_emulated_6131] = { .ATQA = 0x0008, .ATQARelevant = true, .SAK = 0x38, .SAKRelevant = true, .ATSRelevant = false, .Manufacturer = "Nokia", .Type = "MIFARE Classic 4k - emulated (6131 NFC)" }
};

static CardType CardCandidates[ARRAY_COUNT(CardIdentificationList)];
static uint8_t CardCandidatesIdx = 0;

uint16_t addParityBits(uint8_t *Buffer, uint16_t BitCount) {
    if (BitCount == 7)
        return 7;
    if (BitCount % 8)
        return BitCount;
    uint8_t *currByte, * tmpByte;
    uint8_t *const lastByte = Buffer + BitCount / 8 + BitCount / 64; // starting address + number of bytes + number of parity bytes
    currByte = Buffer + BitCount / 8 - 1;
    uint8_t parity;
    memset(currByte + 1, 0, lastByte - currByte); // zeroize all bytes used for parity bits
    while (currByte >= Buffer) { // loop over all input bytes
        parity = OddParityBit(*currByte); // get parity bit
        tmpByte = lastByte;
        while (tmpByte > currByte) { // loop over all bytes from the last byte to the current one -- shifts the whole byte string
            *tmpByte <<= 1; // shift this byte
            *tmpByte |= (*(tmpByte - 1) & 0x80) >> 7; // insert the last bit from the previous byte
            tmpByte--; // go to the previous byte
        }
        *(++tmpByte) &= 0xFE; // zeroize the bit, where we want to put the parity bit
        *tmpByte |= parity & 1; // add the parity bit
        currByte--; // go to previous input byte
    }
    return BitCount + (BitCount / 8);
}

uint16_t removeParityBits(uint8_t *Buffer, uint16_t BitCount) {
    // Short frame, no parity bit is added
    if (BitCount == 7)
        return 7;

    uint16_t i;
    for (i = 0; i < (BitCount / 9); i++) {
        Buffer[i] = (Buffer[i + i / 8] >> (i % 8));
        if (i % 8)
            Buffer[i] |= (Buffer[i + i / 8 + 1] << (8 - (i % 8)));
    }
    return BitCount / 9 * 8;
}

bool checkParityBits(uint8_t *Buffer, uint16_t BitCount) {
    if (BitCount == 7)
        return true;

    //if (BitCount % 9 || BitCount == 0)
    //	return false;

    uint16_t i;
    uint8_t currentByte, parity;
    for (i = 0; i < (BitCount / 9); i++) {
        currentByte = (Buffer[i + i / 8] >> (i % 8));
        if (i % 8)
            currentByte |= (Buffer[i + i / 8 + 1] << (8 - (i % 8)));
        parity = OddParityBit(currentByte);
        if (((Buffer[i + i / 8 + 1] >> (i % 8)) ^ parity) & 1) {
            return false;
        }
    }
    return true;
}

void Reader14443AAppTimeout(void) {
    Reader14443AAppReset();
    Reader14443ACodecReset();
    ReaderState = STATE_IDLE;
}

void Reader14443AAppInit(void) {
    ReaderState = STATE_IDLE;
}

void Reader14443AAppReset(void) {
    ReaderState = STATE_IDLE;
    Reader14443CurrentCommand = Reader14443_Do_Nothing;
    Selected = false;
}

void Reader14443AAppTask(void) {

}

void Reader14443AAppTick(void) {

}

static uint16_t Reader14443A_Deselect(uint8_t *Buffer) { // deselects the card because of an error, so we will continue to select the card afterwards
    Buffer[0] = 0xC2;
    ISO14443AAppendCRCA(Buffer, 1);
    ReaderState = STATE_DESELECT;
    Selected = false;
    return addParityBits(Buffer, 24);
}

static uint16_t Reader14443A_Select(uint8_t *Buffer, uint16_t BitCount) {
    if (Selected) {
        if (ReaderState > STATE_HALT)
            return 0;
        else
            Selected = false;
    }

    // general frame handling:
    uint8_t flags = 0;
    if (BitCount > 0 && checkParityBits(Buffer, BitCount)) {
        flags |= FLAGS_PARITY_OK;
        BitCount = removeParityBits(Buffer, BitCount);
    } else if (BitCount == 0) {
        flags |= FLAGS_NO_DATA;
    } else { // checkParityBits returned false
        LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (BitCount + 7) / 8);
    }


    switch (ReaderState) {
        case STATE_IDLE:
        case STATE_HALT:
            Reader_FWT = 4;
            /* Send a REQA */
            Buffer[0] = ISO14443A_CMD_WUPA; // whenever REQA works, WUPA also works, so we choose WUPA always
            ReaderState = STATE_READY;
            return 7;

        case STATE_READY:
            if (BitCount != 16 || (flags & FLAGS_PARITY_OK) == 0) {
                ReaderState = STATE_IDLE;
                Reader14443ACodecStart();
                return 0;
            }
            CardCharacteristics.ATQA = Buffer[1] << 8 | Buffer[0]; // save ATQA for possible later use
            Buffer[0] = ISO14443A_CMD_SELECT_CL1;
            Buffer[1] = 0x20; // NVB = 16
            ReaderState = STATE_ACTIVE_CL1;
            return addParityBits(Buffer, 2 * BITS_PER_BYTE);

        case STATE_ACTIVE_CL1 ... STATE_ACTIVE_CL3:
            if ((flags & FLAGS_PARITY_OK) == 0 || BitCount < (5 * BITS_PER_BYTE) || !CHECK_BCC(Buffer)) {
                ReaderState = STATE_IDLE;
                Reader14443ACodecStart();
                return 0;
            }

            if (Buffer[0] == ISO14443A_UID0_CT) {
                memcpy(CardCharacteristics.UID + (ReaderState - STATE_ACTIVE_CL1) * 3, Buffer + 1, 3);
            } else {
                memcpy(CardCharacteristics.UID + (ReaderState - STATE_ACTIVE_CL1) * 3, Buffer, 4);
            }
            // shift received UID two bytes to the right
            memmove(Buffer + 2, Buffer, 5);
            Buffer[0] = (ReaderState == STATE_ACTIVE_CL1) ? ISO14443A_CMD_SELECT_CL1 : (ReaderState == STATE_ACTIVE_CL2) ? ISO14443A_CMD_SELECT_CL2 : ISO14443A_CMD_SELECT_CL3;
            Buffer[1] = 0x70; // NVB = 56
            ISO14443AAppendCRCA(Buffer, 7);
            ReaderState = ReaderState - STATE_ACTIVE_CL1 + STATE_SAK_CL1;
            return addParityBits(Buffer, (7 + 2) * BITS_PER_BYTE);

        case STATE_SAK_CL1 ... STATE_SAK_CL3:
            if ((flags & FLAGS_PARITY_OK) == 0 || BitCount != (3 * BITS_PER_BYTE) || ISO14443_CRCA(Buffer, 3) != 0) {
                ReaderState = STATE_IDLE;
                Reader14443ACodecStart();
                return 0;
            }

            if (IS_CASCADE_BIT_SET(Buffer) && ReaderState != STATE_SAK_CL3) {
                Buffer[0] = (ReaderState == STATE_SAK_CL1) ? ISO14443A_CMD_SELECT_CL2 : ISO14443A_CMD_SELECT_CL3;
                Buffer[1] = 0x20; // NVB = 16 bit
                ReaderState = ReaderState - STATE_SAK_CL1 + STATE_ACTIVE_CL1 + 1;
                return addParityBits(Buffer, 2 * BITS_PER_BYTE);
            } else if (IS_CASCADE_BIT_SET(Buffer) && ReaderState == STATE_SAK_CL3) {
                // TODO handle this very strange hopefully not happening error
            }
            Selected = true;
            CardCharacteristics.UIDSize = (ReaderState - STATE_SAK_CL1) * 3 + 4;
            CardCharacteristics.SAK = Buffer[0]; // save last SAK for possible later use
            return 0;

        case STATE_DESELECT:
            if ((flags & FLAGS_NO_DATA) != 0) { // most likely the card already understood the deselect
                ReaderState = STATE_HALT;
                Reader14443ACodecStart();
                return 0;
            }
            if ((flags & FLAGS_PARITY_OK) == 0 || ISO14443_CRCA(Buffer, 3)) {
                return Reader14443A_Deselect(Buffer);
            }
            ReaderState = STATE_HALT;
            Reader14443ACodecStart();
            return 0;

        default:
            return 0;
    }
}

INLINE uint16_t Reader14443A_Halt(uint8_t *Buffer) {
    Buffer[0] = ISO14443A_CMD_HLTA;
    Buffer[1] = 0x00;
    ISO14443AAppendCRCA(Buffer, 2);
    ReaderState = STATE_HALT;
    Selected = false;
    return addParityBits(Buffer, 4 * BITS_PER_BYTE);
}

INLINE uint16_t Reader14443A_RATS(uint8_t *Buffer) {
    Buffer[0] = 0xE0; // RATS command
    Buffer[1] = 0x80;
    ISO14443AAppendCRCA(Buffer, 2);
    ReaderState = STATE_ATS;
    return addParityBits(Buffer, 4 * BITS_PER_BYTE);
}

static bool Identify(uint8_t *Buffer, uint16_t *BitCount) {
    uint16_t rVal = Reader14443A_Select(Buffer, *BitCount);
    if (Selected) {
        if (ReaderState >= STATE_SAK_CL1 && ReaderState <= STATE_SAK_CL3) {
            bool ISO14443_4A_compliant = IS_ISO14443A_4_COMPLIANT(Buffer);
            CardCandidatesIdx = 0;

            uint8_t i;
            for (i = 0; i < ARRAY_COUNT(CardIdentificationList); i++) {
                CardIdentificationType card;
                memcpy_P(&card, &CardIdentificationList[i], sizeof(CardIdentificationType));
                if (card.ATQARelevant && card.ATQA != CardCharacteristics.ATQA)
                    continue;
                if (card.SAKRelevant && card.SAK != CardCharacteristics.SAK)
                    continue;
                if (card.ATSRelevant && !ISO14443_4A_compliant)
                    continue; // for this card type candidate, the ATS is relevant, but the card does not support ISO14443-4A
                CardCandidates[CardCandidatesIdx++] = i;
            }

            if (ISO14443_4A_compliant) {
                // send RATS
                *BitCount = Reader14443A_RATS(Buffer);
                return false;
            }
            // if we don't have to send the RATS, we are finished for distinguishing with ISO 14443A

        } else if (ReaderState == STATE_ATS) { // we have got the ATS
            if (!checkParityBits(Buffer, *BitCount)) {
                LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (*BitCount + 8) / 7);
                *BitCount = Reader14443A_Deselect(Buffer);
                return false;
            }
            *BitCount = removeParityBits(Buffer, *BitCount);

            if (Buffer[0] != *BitCount / 8 - 2 || ISO14443_CRCA(Buffer, Buffer[0] + 2)) {
                *BitCount = Reader14443A_Deselect(Buffer);
                return false;
            }

            uint8_t i;
            for (i = 0; i < CardCandidatesIdx; i++) {
                CardIdentificationType card;
                memcpy_P(&card, &CardIdentificationList[CardCandidates[i]], sizeof(CardIdentificationType));
                if (!card.ATSRelevant || (card.ATSRelevant && card.ATSSize == Buffer[0] - 1 && memcmp(card.ATS, Buffer + 1, card.ATSSize) == 0))
                    /*
                     * If for this candidate the ATS is not relevant, it remains being a candidate.
                     * If the ATS is relevant and the size is correct and the ATS is the same as the reference value, this candidate remains a candidate.
                     */
                    continue;

                // Else, we have to delete this candidate
                uint8_t j;
                for (j = i; j < CardCandidatesIdx - 1; j++)
                    CardCandidates[j] = CardCandidates[j + 1];
                CardCandidatesIdx--;
                i--;
            }
        }

        /*
         * If any cards are not distinguishable with ISO14443A commands only, this is the place to run some proprietary commands.
         */

        if ((ReaderState >= STATE_SAK_CL1 && ReaderState <= STATE_SAK_CL3) || ReaderState == STATE_ATS) {
            uint8_t i;
            for (i = 0; i < CardCandidatesIdx; i++) {
                switch (CardCandidates[i]) {
                    case CardType_NXP_MIFARE_DESFire:
                    case CardType_NXP_MIFARE_DESFire_EV1:
                        Buffer[0] = 0x02;
                        Buffer[1] = 0x60;
                        ISO14443AAppendCRCA(Buffer, 2);
                        ReaderState = STATE_DESFIRE_INFO;
                        *BitCount = addParityBits(Buffer, 4 * BITS_PER_BYTE);
                        return false;
#if 0
                    case CardType_NXP_MIFARE_Ultralight:
                    case CardType_NXP_MIFARE_Ultralight_C:
                    case CardType_NXP_MIFARE_Ultralight_EV1:
                        Buffer[0] = 0x1A; // UL C Authenticate
                        Buffer[1] = 0x00;
                        ISO14443AAppendCRCA(Buffer, 2);
                        ReaderState = STATE_UL_C_AUTH;
                        *BitCount = addParityBits(Buffer, 4 * BITS_PER_BYTE);
                        return false;
#endif
                    default:
                        break;
                }
            }
        } else {
            switch (ReaderState) {
                case STATE_DESFIRE_INFO:
                    if (*BitCount == 0) {
                        CardCandidatesIdx = 0; // this will return that this card is unknown to us
                        break;
                    }
                    if (!checkParityBits(Buffer, *BitCount)) {
                        LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (*BitCount + 8) / 7);
                        CardCandidatesIdx = 0;
                        *BitCount = Reader14443A_Deselect(Buffer);
                        return false;
                    }
                    *BitCount = removeParityBits(Buffer, *BitCount);
                    if (ISO14443_CRCA(Buffer, *BitCount / 8)) {
                        CardCandidatesIdx = 0;
                        *BitCount = Reader14443A_Deselect(Buffer);
                        return false;
                    }
                    switch (Buffer[5]) {
                        case 0x00:
                            CardCandidatesIdx = 1;
                            CardCandidates[0] = CardType_NXP_MIFARE_DESFire;
                            break;

                        case 0x01:
                            CardCandidatesIdx = 1;
                            CardCandidates[0] = CardType_NXP_MIFARE_DESFire_EV1;
                            break;

                        default:
                            CardCandidatesIdx = 0;
                    }
                    break;
#if 0
                case STATE_UL_C_AUTH:
                    if (*BitCount == 0) {
                        Buffer[0] = 0x60; // Get Version command for UL EV1
                        ISO14443AAppendCRCA(Buffer, 1);
                        *BitCount = addParityBits(Buffer, 3 * BITS_PER_BYTE);
                        ReaderState = STATE_UL_EV1_GETVERSION;
                        return false;
                    }
                    CardCandidatesIdx = 1;
                    CardCandidates[0] = CardType_NXP_MIFARE_Ultralight_C;
                    break;

                case STATE_UL_EV1_GETVERSION:
                    if (*BitCount == 0) {
                        CardCandidatesIdx = 1;
                        CardCandidates[0] = CardType_NXP_MIFARE_Ultralight;
                        return true;
                    }
                    CardCandidatesIdx = 1;
                    CardCandidates[0] = CardType_NXP_MIFARE_Ultralight_EV1;
                    break;
#endif
                default:
                    break;
            }
        }
        return true;
    }
    *BitCount = rVal;
    return false;
}

uint16_t Reader14443AAppProcess(uint8_t *Buffer, uint16_t BitCount) {
    switch (Reader14443CurrentCommand) {
        case Reader14443_Send: {
            if (ReaderSendBitCount) {
                memcpy(Buffer, ReaderSendBuffer, (ReaderSendBitCount + 7) / 8);
                uint16_t tmp = addParityBits(Buffer, ReaderSendBitCount);
                ReaderSendBitCount = 0;
                return tmp;
            }

            if (BitCount == 0) {
                char tmpBuf[] = "NO DATA";
                Reader14443CurrentCommand = Reader14443_Do_Nothing;
                CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
                return 0;
            }
            char tmpBuf[128];
            bool parity = checkParityBits(Buffer, BitCount);
            BitCount = removeParityBits(Buffer, BitCount);
            if ((2 * (BitCount + 7) / 8 + 2 + 4) > 128) { // 2 = \r\n, 4 = size of bitcount in hex
                sprintf(tmpBuf, "Too many data.");
                Reader14443CurrentCommand = Reader14443_Do_Nothing;
                CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
                return 0;
            }
            uint16_t charCnt = BufferToHexString(tmpBuf, 128, Buffer, (BitCount + 7) / 8);
            uint8_t count[2] = {(BitCount >> 8) & 0xFF, BitCount & 0xFF};
            charCnt += snprintf(tmpBuf + charCnt, 128 - charCnt, "\r\n");
            charCnt += BufferToHexString(tmpBuf + charCnt, 128 - charCnt, count, 2);
            if (!parity)
                snprintf(tmpBuf + charCnt, 128 - charCnt, "\r\nPARITY ERROR");
            else
                snprintf(tmpBuf + charCnt, 128 - charCnt, "\r\nPARITY OK");
            Reader14443CurrentCommand = Reader14443_Do_Nothing;
            CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
            return 0;
        }


        case Reader14443_Send_Raw: {
            if (ReaderSendBitCount) {
                memcpy(Buffer, ReaderSendBuffer, (ReaderSendBitCount + 7) / 8);
                uint16_t tmp = ReaderSendBitCount;
                ReaderSendBitCount = 0;
                return tmp;
            }

            if (BitCount == 0) {
                char tmpBuf[] = "NO DATA";
                Reader14443CurrentCommand = Reader14443_Do_Nothing;
                CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
                return 0;
            }

            char tmpBuf[128];
            uint16_t charCnt = BufferToHexString(tmpBuf, 128, Buffer, (BitCount + 7) / 8);
            uint8_t count[2] = {(BitCount >> 8) & 0xFF, BitCount & 0xFF};
            charCnt += snprintf(tmpBuf + charCnt, 128 - charCnt, "\r\n");
            charCnt += BufferToHexString(tmpBuf + charCnt, 128 - charCnt, count, 2);
            Reader14443CurrentCommand = Reader14443_Do_Nothing;
            CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
            return 0;
        }

        case Reader14443_Get_UID: {
            uint16_t rVal = Reader14443A_Select(Buffer, BitCount);
            if (Selected) { // we are done finding the UID
                char tmpBuf[20];
                BufferToHexString(tmpBuf, 20, CardCharacteristics.UID, CardCharacteristics.UIDSize);
                CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
                Selected = false;
                Reader14443CurrentCommand = Reader14443_Do_Nothing;
                CodecReaderFieldStop();
                return 0;
            }
            return rVal;
        }

        case Reader14443_Autocalibrate: {
            static enum {
                RT_STATE_IDLE,
                RT_STATE_SEARCHING
            } RTState = RT_STATE_IDLE;
            static uint8_t TryCount = 0;
            static uint8_t Thresholds[(CODEC_THRESHOLD_CALIBRATE_MAX - CODEC_THRESHOLD_CALIBRATE_MIN) / CODEC_THRESHOLD_CALIBRATE_STEPS] = {0};

            if (RTState == RT_STATE_IDLE) {
                CodecThresholdSet(CODEC_THRESHOLD_CALIBRATE_MIN);
                RTState = RT_STATE_SEARCHING;
                TryCount = 0;
            } else if (RTState == RT_STATE_SEARCHING && ReaderState <= STATE_HALT) {
                if (++TryCount == TRYCOUNT_MAX) {
                    uint16_t tmp_th = CodecThresholdIncrement();
                    if ((tmp_th >= CODEC_THRESHOLD_CALIBRATE_MID && (tmp_th - CODEC_THRESHOLD_CALIBRATE_STEPS) < CODEC_THRESHOLD_CALIBRATE_MID)
                            ||
                            tmp_th >= CODEC_THRESHOLD_CALIBRATE_MAX) {
                        bool block = false, finished = false;
                        uint16_t min = 0;
                        uint16_t max = 0;
                        uint16_t maxdiff = 0;
                        uint16_t maxdiffoffset = 0;
                        uint16_t numworked;
                        uint16_t i;

                        // first, search inside the usual search space
                        if (tmp_th >= CODEC_THRESHOLD_CALIBRATE_MID && (tmp_th - CODEC_THRESHOLD_CALIBRATE_STEPS) < CODEC_THRESHOLD_CALIBRATE_MID) {
                            for (i = 0; i < (CODEC_THRESHOLD_CALIBRATE_MID - CODEC_THRESHOLD_CALIBRATE_MIN) / CODEC_THRESHOLD_CALIBRATE_STEPS; i++) {
                                if (Thresholds[i] == TRYCOUNT_MAX && i < ((CODEC_THRESHOLD_CALIBRATE_MID - CODEC_THRESHOLD_CALIBRATE_MIN) / CODEC_THRESHOLD_CALIBRATE_STEPS - 1)) {
                                    if (!block) {
                                        block = true;
                                        min = i;
                                    }
                                } else {
                                    if (block) {
                                        block = false;
                                        max = i;
                                        if ((max - min) >= maxdiff) {
                                            maxdiff = max - min;
                                            maxdiffoffset = min;
                                        }
                                    }
                                }
                            }
                            if (maxdiff >= 4) { // if we have found something with at least 5 consecutive working thresholds (only if these thresholds have worked for evers attempt), we are done
                                finished = true;
                            }
                        } else { // we have searched the whole space
                            for (numworked = TRYCOUNT_MAX; numworked > 0; numworked--) {
                                for (i = 0; i < (CODEC_THRESHOLD_CALIBRATE_MAX - CODEC_THRESHOLD_CALIBRATE_MIN) / CODEC_THRESHOLD_CALIBRATE_STEPS; i++) {
                                    if (Thresholds[i] >= numworked) {
                                        if (!block) {
                                            block = true;
                                            min = i;
                                        }
                                    } else {
                                        if (block) {
                                            block = false;
                                            max = i;
                                            if ((max - min) >= maxdiff) {
                                                maxdiff = max - min;
                                                maxdiffoffset = min;
                                            }
                                        }
                                    }
                                }
                                if (maxdiff > 0) {
                                    break;
                                }
                            }

                            finished = true;
                        }

                        if (finished) {

                            RTState = RT_STATE_IDLE;

                            if (maxdiff != 0)
                                CodecThresholdSet((maxdiffoffset + maxdiff / 2) * CODEC_THRESHOLD_CALIBRATE_STEPS + CODEC_THRESHOLD_CALIBRATE_MIN);
                            else
                                CodecThresholdReset();
                            SETTING_UPDATE(GlobalSettings.ActiveSettingPtr->ReaderThreshold);

                            CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, NULL);
                            uint16_t i_max = (CODEC_THRESHOLD_CALIBRATE_MID - CODEC_THRESHOLD_CALIBRATE_MIN) / CODEC_THRESHOLD_CALIBRATE_STEPS;
                            if (tmp_th >= CODEC_THRESHOLD_CALIBRATE_MAX)
                                i_max = (CODEC_THRESHOLD_CALIBRATE_MAX - CODEC_THRESHOLD_CALIBRATE_MIN) / CODEC_THRESHOLD_CALIBRATE_STEPS;
                            for (i = 0; i < i_max; i++) {
                                char tmpBuf[10];
                                snprintf(tmpBuf, 10, "%4" PRIu16 ": ", i * CODEC_THRESHOLD_CALIBRATE_STEPS + CODEC_THRESHOLD_CALIBRATE_MIN);
                                TerminalSendString(tmpBuf);
                                if (Thresholds[i]) {
                                    snprintf(tmpBuf, 10, "%3" PRIu16, Thresholds[i]);
                                    TerminalSendString(tmpBuf);
                                } else {
                                    TerminalSendChar('-');
                                }
                                TerminalSendStringP(PSTR("\r\n"));
                                Thresholds[i] = 0; // reset the threshold so the next run won't show old results
                            }


                            Selected = false;
                            Reader14443CurrentCommand = Reader14443_Do_Nothing;
                            Reader14443ACodecReset();
                            return 0;
                        }
                    }
                    TryCount = 0;
                }
            }

            uint16_t rVal = Reader14443A_Select(Buffer, BitCount);
            if (Selected) { // we are done finding the threshold
                Thresholds[(GlobalSettings.ActiveSettingPtr->ReaderThreshold - CODEC_THRESHOLD_CALIBRATE_MIN) / CODEC_THRESHOLD_CALIBRATE_STEPS] += 1;
                if (TryCount == TRYCOUNT_MAX) {
                    CodecThresholdIncrement();
                    TryCount = 0;
                }
                ReaderState = STATE_IDLE;
                Reader14443ACodecStart();
                return Reader14443A_Halt(Buffer);
            }
            return rVal;
        }

        case Reader14443_Clone_MF_Ultralight:
        case Reader14443_Read_MF_Ultralight: {
            static uint8_t MFURead_CurrentAdress = 0;
            static uint8_t MFUContents[64];

            uint16_t rVal = Reader14443A_Select(Buffer, BitCount);
            if (Selected) {
                if (MFURead_CurrentAdress != 0) {
                    if (BitCount == 0) { // relaunch select protocol
                        MFURead_CurrentAdress = 0; // reset read address
                        Selected = false;
                        ReaderState = STATE_IDLE;
                        Reader14443ACodecStart();
                        return 0;
                    }
                    bool readPageAgain = (BitCount < 162) || !checkParityBits(Buffer, BitCount);
                    BitCount = removeParityBits(Buffer, BitCount);
                    if (readPageAgain || ISO14443_CRCA(Buffer, 18)) { // the CRC function should return 0 if everything is ok
                        MFURead_CurrentAdress -= 4;
                    } else { // everything is ok for this page
                        memcpy(MFUContents + (MFURead_CurrentAdress - 4) * 4, Buffer, 16);
                    }
                } else {
                    uint16_t RefATQA;
                    memcpy_P(&RefATQA, &CardIdentificationList[CardType_NXP_MIFARE_Ultralight].ATQA, 2);
                    uint8_t RefSAK = pgm_read_byte(&CardIdentificationList[CardType_NXP_MIFARE_Ultralight].SAK);
                    if (CardCharacteristics.ATQA != RefATQA || CardCharacteristics.SAK != RefSAK) { // seems to be no MiFare Ultralight card, so retry
                        ReaderState = STATE_IDLE;
                        Reader14443ACodecStart();
                        return 0;
                    }
                }
                if (MFURead_CurrentAdress == 16) {
                    Selected = false;
                    MFURead_CurrentAdress = 0;

                    if (Reader14443CurrentCommand == Reader14443_Read_MF_Ultralight) { // dump
                        Reader14443CurrentCommand = Reader14443_Do_Nothing;
                        char tmpBuf[135]; // 135 = 128 hex digits + 3 * \r\n + \0
                        BufferToHexString(tmpBuf, 							135, 							MFUContents, 16);
                        snprintf(tmpBuf + 32, 						135 - 32, 						"\r\n");
                        BufferToHexString(tmpBuf + 32 + 2, 					135 - 32 - 2, 					MFUContents + 16, 16);
                        snprintf(tmpBuf + 32 + 2 + 32, 				135 - 32 - 2 - 32, 				"\r\n");
                        BufferToHexString(tmpBuf + 32 + 2 + 32 + 2, 			135 - 32 - 2 - 32 - 2, 			MFUContents + 32, 16);
                        snprintf(tmpBuf + 32 + 2 + 32 + 2 + 32, 		135 - 32 - 2 - 32 - 2 - 32, 	"\r\n");
                        BufferToHexString(tmpBuf + 32 + 2 + 32 + 2 + 32 + 2, 	135 - 32 - 2 - 32 - 2 - 32 - 2, MFUContents + 48, 16);
                        CodecReaderFieldStop();
                        CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
                    } else { // clone
                        Reader14443CurrentCommand = Reader14443_Do_Nothing;
                        CodecReaderFieldStop();
                        MemoryUploadBlock(&MFUContents, 0, 64);
                        CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, "Card Cloned to Slot");
                        ConfigurationSetById(CONFIG_MF_ULTRALIGHT, false);
                        MemoryStore();
                        SettingsSave();
                    }
                    return 0;
                }
                Buffer[0] = 0x30; // MiFare Ultralight read command
                Buffer[1] = MFURead_CurrentAdress;
                ISO14443AAppendCRCA(Buffer, 2);

                MFURead_CurrentAdress += 4;

                return addParityBits(Buffer, 4 * BITS_PER_BYTE);
            }
            return rVal;
        }

        /************************************
         * This function identifies a PICC. *
         ************************************/
        case Reader14443_Identify: {
            if (Identify(Buffer, &BitCount)) {
                if (CardCandidatesIdx == 0) {
                    CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, "Unknown card type.");
                } else if (CardCandidatesIdx == 1) {
                    char tmpType[64];
                    memcpy_P(tmpType, &CardIdentificationList[CardCandidates[0]].Type, 64);
                    CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpType);
                } else {
                    char tmpBuf[TERMINAL_BUFFER_SIZE];
                    uint16_t size = 0, tmpsize = 0;
                    bool enoughspace = true;

                    uint8_t i;
                    for (i = 0; i < CardCandidatesIdx; i++) {
                        if (size <= TERMINAL_BUFFER_SIZE) { // prevents buffer overflow
                            char tmpType[64];
                            memcpy_P(tmpType, &CardIdentificationList[CardCandidates[i]].Type, 64);
                            tmpsize = snprintf(tmpBuf + size, TERMINAL_BUFFER_SIZE - size, "%s or ", tmpType);
                            size += tmpsize;
                        } else {
                            break;
                        }
                    }
                    if (size > TERMINAL_BUFFER_SIZE) {
                        size -= tmpsize;
                        enoughspace = false;
                    }
                    tmpBuf[size - 4] = '.';
                    tmpBuf[size - 3] = '\0';
                    CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
                    if (!enoughspace)
                        TerminalSendStringP(PSTR("There is at least one more card type candidate, but there was not enough terminal buffer space.\r\n"));
                }
                // print general data
                TerminalSendStringP(PSTR("ATQA:\t"));
                CommandLineAppendData(&CardCharacteristics.ATQA, 2);
                TerminalSendStringP(PSTR("UID:\t"));
                CommandLineAppendData(CardCharacteristics.UID, CardCharacteristics.UIDSize);
                TerminalSendStringP(PSTR("SAK:\t"));
                CommandLineAppendData(&CardCharacteristics.SAK, 1);

                Reader14443CurrentCommand = Reader14443_Do_Nothing;
                CardCandidatesIdx = 0;
                CodecReaderFieldStop();
                Selected = false;
                return 0;
            } else {
                return BitCount;
            }
        }
        /****************************************
         * This function do simple cloning UID. *
         ****************************************/
        case Reader14443_Identify_Clone: {
            if (Identify(Buffer, &BitCount)) {
                if (CardCandidatesIdx == 1) {
                    int cfgid = -1;
                    switch (CardCandidates[0]) {
                        case CardType_NXP_MIFARE_Ultralight: {
#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
                            cfgid = CONFIG_MF_ULTRALIGHT;
#endif
                            // TODO: enter MFU clone mdoe
                            break;
                        }
                        case CardType_NXP_MIFARE_DESFire_EV1: {
#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
                            cfgid = CONFIG_MF_ULTRALIGHT;
#endif
                            // Only set UL for DESFire_EV1 and read UID for some small tests - simple UID cloning
                            break;
                        }
                        case CardType_NXP_MIFARE_Classic_1k:
                        case CardType_Infineon_MIFARE_Classic_1k: {
                            if (CardCharacteristics.UIDSize == UIDSize_Single) {
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
                                cfgid = CONFIG_MF_CLASSIC_1K;
#endif
                            } else if (CardCharacteristics.UIDSize == UIDSize_Double) {
#ifdef CONFIG_MF_CLASSIC_1K_7B_SUPPORT
                                cfgid = CONFIG_MF_CLASSIC_1K_7B;
#endif
                            }
                            break;
                        }
                        case CardType_NXP_MIFARE_Classic_4k:
                        case CardType_Nokia_MIFARE_Classic_4k_emulated_6212:
                        case CardType_Nokia_MIFARE_Classic_4k_emulated_6131: {
                            if (CardCharacteristics.UIDSize == UIDSize_Single) {
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
                                cfgid = CONFIG_MF_CLASSIC_4K;
#endif
                            } else if (CardCharacteristics.UIDSize == UIDSize_Double) {
#ifdef CONFIG_MF_CLASSIC_4K_7B_SUPPORT
                                cfgid = CONFIG_MF_CLASSIC_4K_7B;
#endif
                            }
                            break;
                        }
                        default:
                            cfgid = -1;
                    }

                    if (cfgid > -1) { /* TODO: Consider wrapping strings with PSTR(...): */
                        CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, "Cloned OK!");
                        /* Notify LED. blink when clone is done - ToDo: maybe use other LEDHook */
                        LEDHook(LED_SETTING_CHANGE, LED_BLINK_2X);
                        ConfigurationSetById(cfgid, false);
                        ApplicationReset();
                        ApplicationSetUid(CardCharacteristics.UID);
                        MemoryStore();
                        SettingsSave();
                    } else {
                        CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, "Clone unsupported!");
                    }
                } else {
                    CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, "Multiple possibilities, not clonable!");
                }
                Reader14443CurrentCommand = Reader14443_Do_Nothing;
                CardCandidatesIdx = 0;
                CodecReaderFieldStop();
                Selected = false;
                return 0;
            } else {
                return BitCount;
            }
            return 0;
        }

        default: // e.g. Do_Nothing
            return 0;
    }
    return 0;
}

uint16_t ISO14443_CRCA(uint8_t *Buffer, uint8_t ByteCount) {
    uint8_t *DataPtr = Buffer;
    uint16_t crc = 0x6363;
    uint8_t ch;
    while (ByteCount--) {
        ch = *DataPtr++ ^ crc;
        ch = ch ^ (ch << 4);
        crc = (crc >> 8) ^ (ch << 8) ^ (ch << 3) ^ (ch >> 4);
    }
    return crc;
}

#endif
