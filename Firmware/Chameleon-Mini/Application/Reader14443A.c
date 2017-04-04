#include "Reader14443A.h"
#include "Application.h"
#include "ISO14443-3A.h"
#include "../Codec/Reader14443-2A.h"
#include "Crypto1.h"
#include "../System.h"

#include "../Terminal/Terminal.h"

#define CHECK_BCC(B) ((B[0] ^ B[1] ^ B[2] ^ B[3]) == B[4])
#define IS_CASCADE_BIT_SET(buf) (buf[0] & 0x04)
#define IS_ISO14443A_4_COMPLIANT(buf) (buf[0] & 0x20)

// TODO replace remaining magic numbers

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
		[CardType_NXP_MIFARE_Mini] 				= { .ATQA=0x0004, .ATQARelevant=true, .SAK=0x09, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="NXP", .Type="MIFARE Mini" },
		[CardType_NXP_MIFARE_Classic_1k] 		= { .ATQA=0x0004, .ATQARelevant=true, .SAK=0x08, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="NXP", .Type="MIFARE Classic 1k" },
		[CardType_NXP_MIFARE_Classic_4k] 		= { .ATQA=0x0002, .ATQARelevant=true, .SAK=0x18, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="NXP", .Type="MIFARE Classic 4k" },
		[CardType_NXP_MIFARE_Ultralight] 		= { .ATQA=0x0044, .ATQARelevant=true, .SAK=0x00, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="NXP", .Type="MIFARE Ultralight" },
		// for the following two, setting ATSRelevant to true would cause checking the ATS value, but the NXP paper for distinguishing cards does not recommend this
		[CardType_NXP_MIFARE_DESFire] 			= { .ATQA=0x0344, .ATQARelevant=true, .SAK=0x20, .SAKRelevant=true, .ATSRelevant=false, .ATSSize= 5, .ATS={0x75, 0x77, 0x81, 0x02, 0x80}, .Manufacturer="NXP", .Type="MIFARE DESFire" },
		[CardType_NXP_MIFARE_DESFire_EV1] 		= { .ATQA=0x0344, .ATQARelevant=true, .SAK=0x20, .SAKRelevant=true, .ATSRelevant=false, .ATSSize= 5, .ATS={0x75, 0x77, 0x81, 0x02, 0x80}, .Manufacturer="NXP", .Type="MIFARE DESFire EV1" },
		[CardType_IBM_JCOP31] 					= { .ATQA=0x0304, .ATQARelevant=true, .SAK=0x28, .SAKRelevant=true, .ATSRelevant=true, .ATSSize= 9, .ATS={0x38, 0x77, 0xb1, 0x4a, 0x43, 0x4f, 0x50, 0x33, 0x31}, .Manufacturer="IBM", .Type="JCOP31" },
		[CardType_IBM_JCOP31_v241] 				= { .ATQA=0x0048, .ATQARelevant=true, .SAK=0x20, .SAKRelevant=true, .ATSRelevant=true, .ATSSize=12, .ATS={0x78, 0x77, 0xb1, 0x02, 0x4a, 0x43, 0x4f, 0x50, 0x76, 0x32, 0x34, 0x31}, .Manufacturer="IBM", .Type="JCOP31 v2.4.1" },
		[CardType_IBM_JCOP41_v22] 				= { .ATQA=0x0048, .ATQARelevant=true, .SAK=0x20, .SAKRelevant=true, .ATSRelevant=true, .ATSSize=12, .ATS={0x38, 0x33, 0xb1, 0x4a, 0x43, 0x4f, 0x50, 0x34, 0x31, 0x56, 0x32, 0x32}, .Manufacturer="IBM", .Type="JCOP41 v2.2" },
		[CardType_IBM_JCOP41_v231] 				= { .ATQA=0x0004, .ATQARelevant=true, .SAK=0x28, .SAKRelevant=true, .ATSRelevant=true, .ATSSize=13, .ATS={0x38, 0x33, 0xb1, 0x4a, 0x43, 0x4f, 0x50, 0x34, 0x31, 0x56, 0x32, 0x33, 0x31}, .Manufacturer="IBM", .Type="JCOP41 v2.3.1" },
		[CardType_Infineon_MIFARE_Classic_1k] 	= { .ATQA=0x0004, .ATQARelevant=true, .SAK=0x88, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="Infineon", .Type="MIFARE Classic 1k" },
		[CardType_Gemplus_MPCOS] 				= { .ATQA=0x0002, .ATQARelevant=true, .SAK=0x98, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="Gemplus", .Type="MPCOS" },
		[CardType_Innovision_Jewel] 			= { .ATQA=0x0C00, .ATQARelevant=true, .SAKRelevant=false, .ATSRelevant=false, .Manufacturer="Innovision R&T", .Type="Jewel" },
		[CardType_Nokia_MIFARE_Classic_4k_emulated_6212] = { .ATQA=0x0002, .ATQARelevant=true, .SAK=0x38, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="Nokia", .Type="MIFARE Classic 4k - emulated (6212 Classic)" },
		[CardType_Nokia_MIFARE_Classic_4k_emulated_6131] = { .ATQA=0x0008, .ATQARelevant=true, .SAK=0x38, .SAKRelevant=true, .ATSRelevant=false, .Manufacturer="Nokia", .Type="MIFARE Classic 4k - emulated (6131 NFC)" }
};

static CardType CardCandidates[ARRAY_COUNT(CardIdentificationList)];
static uint8_t CardCandidatesIdx = 0;

uint16_t addParityBits(uint8_t * Buffer, uint16_t BitCount)
{
	uint8_t i = CardIdentificationList[0].ATQA;
	if (i == 0)
		return 0;
	if (BitCount == 7)
		return 7;
	if (BitCount % 8)
		return BitCount;
	uint8_t * currByte, * tmpByte;
	uint8_t * const lastByte = Buffer + BitCount/8 + BitCount/64; // starting address + number of bytes + number of parity bytes
	currByte = Buffer + BitCount/8 - 1;
	uint8_t parity;
	memset(currByte+1, 0, lastByte-currByte); // zeroize all bytes used for parity bits
	while (currByte >= Buffer) // loop over all input bytes
	{
		parity = OddParityBit(*currByte); // get parity bit
		tmpByte = lastByte;
		while (tmpByte > currByte) // loop over all bytes from the last byte to the current one -- shifts the whole byte string
		{
			*tmpByte <<= 1; // shift this byte
			*tmpByte |= (*(tmpByte-1) & 0x80) >> 7; // insert the last bit from the previous byte
			tmpByte--; // go to the previous byte
		}
		*(++tmpByte) &= 0xFE; // zeroize the bit, where we want to put the parity bit
		*tmpByte |= parity & 1; // add the parity bit
		currByte--; // go to previous input byte
	}
	return BitCount + (BitCount / 8);
}

uint16_t removeParityBits(uint8_t * Buffer, uint16_t BitCount)
{
	if (BitCount == 7)
		return 7;

	uint16_t i;
	for (i = 0; i < (BitCount / 9); i++)
	{
		Buffer[i] = (Buffer[i + i/8] >> (i%8));
		if (i%8)
			Buffer[i] |= (Buffer[i + i/8 + 1] << (8 - (i % 8)));
	}
	return BitCount/9*8;
}

uint16_t removeSOC(uint8_t * Buffer, uint16_t BitCount)
{
	if (BitCount == 0)
		return 0;
	uint16_t i;
	Buffer[0] >>= 1;
	for (i = 1; i < (BitCount + 7) / 8; i++)
	{
		Buffer[i-1] |= Buffer[i] << 7;
		Buffer[i] >>= 1;
	}
	return BitCount - 1;
}

bool checkParityBits(uint8_t * Buffer, uint16_t BitCount)
{
	if (BitCount == 7)
		return true;

	//if (BitCount % 9 || BitCount == 0)
	//	return false;

	uint16_t i;
	uint8_t currentByte, parity;
	for (i = 0; i < (BitCount / 9); i++)
	{
		currentByte = (Buffer[i + i/8] >> (i%8));
		if (i%8)
			currentByte |= (Buffer[i + i/8 + 1] << (8 - (i % 8)));
		parity = OddParityBit(currentByte);
		if (((Buffer[i + i/8 + 1] >> (i % 8)) ^ parity) & 1) {
			return false;
		}
	}
	return true;
}

void Reader14443AAppTimeout(void)
{
	Reader14443AAppReset();
	Reader14443ACodecReset();
	ReaderState = STATE_IDLE;
}

void Reader14443AAppInit(void)
{
	ReaderState = STATE_IDLE;
}

void Reader14443AAppReset(void)
{
	ReaderState = STATE_IDLE;
	Reader14443CurrentCommand = Reader14443_Do_Nothing;
	Selected = false;
}

void Reader14443AAppTask(void)
{

}

void Reader14443AAppTick(void)
{

}

INLINE uint16_t Reader14443A_Deselect(uint8_t* Buffer) // deselects the card because of an error, so we will continue to select the card afterwards
{
	Buffer[0] = 0xC2;
	uint16_t crc = ISO14443_CRCA(Buffer, 1);
	Buffer[1] = crc;
	Buffer[2] = crc >> 8;
	ReaderState = STATE_DESELECT;
	Selected = false;
	return addParityBits(Buffer, 24);
}

INLINE uint16_t Reader14443A_Select(uint8_t * Buffer, uint16_t BitCount)
{
	if (Selected)
	{
		if (ReaderState > STATE_HALT)
			return 0;
		else
			Selected = false;
	}
	switch (ReaderState)
	{
	case STATE_IDLE:
	case STATE_HALT:
		Reader_FWT = ISO14443A_RX_PENDING_TIMEOUT;
		/* Send a REQA */
		Buffer[0] = ISO14443A_CMD_WUPA; // whenever REQA works, WUPA also works, so we choose WUPA always
		ReaderState = STATE_READY;
		return 7;

	case STATE_READY:
		if (BitCount < 19)
		{
			ReaderState = STATE_IDLE;
			Reader14443ACodecStart();
			return 0;
		}
		BitCount = removeSOC(Buffer, BitCount);
		if (!checkParityBits(Buffer, BitCount))
		{
			ReaderState = STATE_IDLE;
			LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (BitCount + 8) / 7);
			Reader14443ACodecStart();
			return 0;
		}
		BitCount = removeParityBits(Buffer, BitCount);
		CardCharacteristics.ATQA = Buffer[1] << 8 | Buffer[0]; // save ATQA for possible later use
		Buffer[0] = ISO14443A_CMD_SELECT_CL1;
		Buffer[1] = 0x20; // NVB = 16
		ReaderState = STATE_ACTIVE_CL1;
		return addParityBits(Buffer, 16);

	case STATE_ACTIVE_CL1 ... STATE_ACTIVE_CL3:
		BitCount = removeSOC(Buffer, BitCount);
		if (!checkParityBits(Buffer, BitCount) || BitCount < 8)
		{
			ReaderState = STATE_IDLE;
			LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (BitCount + 8) / 7);
			Reader14443ACodecStart();
			return 0;
		}
		BitCount = removeParityBits(Buffer, BitCount);
		if (!CHECK_BCC(Buffer))
		{
			ReaderState = STATE_IDLE;
			Reader14443ACodecStart();
			return 0;
		}

		if (Buffer[0] == ISO14443A_UID0_CT)
		{
			memcpy(CardCharacteristics.UID + (ReaderState - STATE_ACTIVE_CL1) * 3, Buffer + 1, 3);
		} else {
			memcpy(CardCharacteristics.UID + (ReaderState - STATE_ACTIVE_CL1) * 3, Buffer, 4);
		}
		// shift received UID two bytes to the right
		memmove(Buffer+2, Buffer, 5);
		Buffer[0] = (ReaderState == STATE_ACTIVE_CL1) ? ISO14443A_CMD_SELECT_CL1 : (ReaderState == STATE_ACTIVE_CL2) ? ISO14443A_CMD_SELECT_CL2 : ISO14443A_CMD_SELECT_CL3;
		Buffer[1] = 0x70; // NVB = 56
		uint16_t crc = ISO14443_CRCA(Buffer, 7);
		Buffer[7] = crc & 0xFF;
		Buffer[8] = crc >> 8;
		ReaderState = ReaderState - STATE_ACTIVE_CL1 + STATE_SAK_CL1;
	    return addParityBits(Buffer, 72);

	case STATE_SAK_CL1 ... STATE_SAK_CL3:
		if (BitCount < 9)
		{
			ReaderState = STATE_IDLE;
			Reader14443ACodecStart();
			return 0;
		}
		BitCount = removeSOC(Buffer, BitCount);
		if (!checkParityBits(Buffer, BitCount))
		{
			LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (BitCount + 8) / 7);
			ReaderState = STATE_IDLE;
			Reader14443ACodecStart();
			return 0;
		}
		BitCount = removeParityBits(Buffer, BitCount);
		if (ISO14443_CRCA(Buffer, 3))
		{
			ReaderState = STATE_IDLE;
			Reader14443ACodecStart();
			return 0;
		}
		if (IS_CASCADE_BIT_SET(Buffer) && ReaderState != STATE_SAK_CL3)
		{
			Buffer[0] = (ReaderState == STATE_SAK_CL1) ? ISO14443A_CMD_SELECT_CL2 : ISO14443A_CMD_SELECT_CL3;
			Buffer[1] = 0x20; // NVB = 16 bit
			ReaderState = ReaderState - STATE_SAK_CL1 + STATE_ACTIVE_CL1 + 1;
			return addParityBits(Buffer, 16);
		} else if (IS_CASCADE_BIT_SET(Buffer) && ReaderState == STATE_SAK_CL3) {
			// TODO handle this very strange hopefully not happening error
		}
		Selected = true;
		CardCharacteristics.UIDSize = (ReaderState - STATE_SAK_CL1) * 3 + 4;
		CardCharacteristics.SAK = Buffer[0]; // save last SAK for possible later use

		return 0;

	case STATE_DESELECT:
		if (BitCount == 0) // most likely the card already understood the deselect
		{
			ReaderState = STATE_HALT;
			Reader14443ACodecStart();
			return 0;
		}
		BitCount = removeSOC(Buffer, BitCount);
		if (!checkParityBits(Buffer, BitCount))
		{
			LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (BitCount + 8) / 7);
			return Reader14443A_Deselect(Buffer);
		}
		BitCount = removeParityBits(Buffer, BitCount);
		if (ISO14443_CRCA(Buffer, 3))
		{
			return Reader14443A_Deselect(Buffer);
		}
		ReaderState = STATE_HALT;
		Reader14443ACodecStart();
		return 0;

	default:
		return 0;
	}
}

INLINE uint16_t Reader14443A_Halt(uint8_t* Buffer)
{
	Buffer[0] = ISO14443A_CMD_HLTA;
	Buffer[1] = 0x00;
	uint16_t crc = ISO14443_CRCA(Buffer, 2);
	Buffer[2] = crc;
	Buffer[3] = crc >> 8;
	ReaderState = STATE_HALT;
	Selected = false;
	return addParityBits(Buffer, 32);
}

INLINE uint16_t Reader14443A_RATS(uint8_t* Buffer)
{
	Buffer[0] = 0xE0; // RATS command
	Buffer[1] = 0x80;
	uint16_t crc = ISO14443_CRCA(Buffer, 2);
	Buffer[2] = crc;
	Buffer[3] = crc >> 8;
	ReaderState = STATE_ATS;
	return addParityBits(Buffer, 32);
}

uint16_t Reader14443AAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
	switch (Reader14443CurrentCommand)
	{
		case Reader14443_Send:
		{
			if (ReaderSendBitCount)
			{
				memcpy(Buffer, ReaderSendBuffer, (ReaderSendBitCount + 7) / 8);
				uint16_t tmp = addParityBits(Buffer, ReaderSendBitCount);
				ReaderSendBitCount = 0;
				return tmp;
			}

			if (BitCount == 0)
			{
				char tmpBuf[] = "NO DATA";
				Reader14443CurrentCommand = Reader14443_Do_Nothing;
				CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
				return 0;
			}
			char tmpBuf[128];
			BitCount = removeSOC(Buffer, BitCount);
			bool parity = checkParityBits(Buffer, BitCount);
			BitCount = removeParityBits(Buffer, BitCount);
			if ((2 * (BitCount + 7) / 8 + 2 + 4) > 128) // 2 = \r\n, 4 = size of bitcount in hex
			{
				sprintf(tmpBuf, "Too many data.");
				Reader14443CurrentCommand = Reader14443_Do_Nothing;
				CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
				return 0;
			}
			uint16_t charCnt = BufferToHexString(tmpBuf, 128, Buffer, (BitCount + 7) / 8);
			uint8_t count[2] = {(BitCount>>8)&0xFF, BitCount&0xFF};
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


		case Reader14443_Send_Raw:
		{
			if (ReaderSendBitCount)
			{
				memcpy(Buffer, ReaderSendBuffer, (ReaderSendBitCount + 7) / 8);
				uint16_t tmp = ReaderSendBitCount;
				ReaderSendBitCount = 0;
				return tmp;
			}

			if (BitCount == 0)
			{
				char tmpBuf[] = "NO DATA";
				Reader14443CurrentCommand = Reader14443_Do_Nothing;
				CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
				return 0;
			}

			char tmpBuf[128];
			uint16_t charCnt = BufferToHexString(tmpBuf, 128, Buffer, (BitCount + 7) / 8);
			uint8_t count[2] = {(BitCount>>8)&0xFF, BitCount&0xFF};
			charCnt += snprintf(tmpBuf + charCnt, 128 - charCnt, "\r\n");
			charCnt += BufferToHexString(tmpBuf + charCnt, 128 - charCnt, count, 2);
			Reader14443CurrentCommand = Reader14443_Do_Nothing;
			CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
			return 0;
		}

		case Reader14443_Get_UID:
		{
			uint16_t rVal = Reader14443A_Select(Buffer, BitCount);
			if (Selected) // we are done finding the UID
			{
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

		case Reader14443_Read_MF_Ultralight:
		{
			static uint8_t MFURead_CurrentAdress = 0;
			static uint8_t MFUContents[64];

			uint16_t rVal = Reader14443A_Select(Buffer, BitCount);
			if (Selected)
			{
				if (MFURead_CurrentAdress != 0)
				{
					BitCount = removeSOC(Buffer, BitCount);
					if (BitCount == 0) // relaunch select protocol
					{
						MFURead_CurrentAdress = 0; // reset read address
						Selected = false;
						ReaderState = STATE_IDLE;
						Reader14443ACodecStart();
						return 0;
					}
					bool readPageAgain = (BitCount < 162) || !checkParityBits(Buffer, BitCount);
					BitCount = removeParityBits(Buffer, BitCount);
					if (readPageAgain || ISO14443_CRCA(Buffer, 18)) // the CRC function should return 0 if everything is ok
					{
						MFURead_CurrentAdress -= 4;
					} else { // everything is ok for this page
						memcpy(MFUContents + (MFURead_CurrentAdress - 4) * 4, Buffer, 16);
					}
				} else {
					uint16_t RefATQA;
					memcpy_P(&RefATQA, &CardIdentificationList[CardType_NXP_MIFARE_Ultralight].ATQA, 2);
					uint8_t RefSAK = pgm_read_byte(&CardIdentificationList[CardType_NXP_MIFARE_Ultralight].SAK);
					if (CardCharacteristics.ATQA != RefATQA || CardCharacteristics.SAK != RefSAK) // seems to be no MiFare Ultralight card, so retry
					{
						ReaderState = STATE_IDLE;
						Reader14443ACodecStart();
						return 0;
					}
				}
				if (MFURead_CurrentAdress == 16)
				{
					Selected = false;
					MFURead_CurrentAdress = 0;
					Reader14443CurrentCommand = Reader14443_Do_Nothing;

					char tmpBuf[135]; // 135 = 128 hex digits + 3 * \r\n + \0
					BufferToHexString(	tmpBuf, 							135, 							MFUContents, 16);
					snprintf(			tmpBuf + 32, 						135 - 32, 						"\r\n");
					BufferToHexString(	tmpBuf + 32 + 2, 					135 - 32 - 2, 					MFUContents + 16, 16);
					snprintf(			tmpBuf + 32 + 2 + 32, 				135 - 32 - 2 - 32, 				"\r\n");
					BufferToHexString(	tmpBuf + 32 + 2 + 32 + 2, 			135 - 32 - 2 - 32 - 2, 			MFUContents + 32, 16);
					snprintf(			tmpBuf + 32 + 2 + 32 + 2 + 32, 		135 - 32 - 2 - 32 - 2 - 32, 	"\r\n");
					BufferToHexString(	tmpBuf + 32 + 2 + 32 + 2 + 32 + 2, 	135 - 32 - 2 - 32 - 2 - 32 - 2, MFUContents + 48, 16);
					CodecReaderFieldStop();
					CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
					return 0;
				}
				Buffer[0] = 0x30; // MiFare Ultralight read command
				Buffer[1] = MFURead_CurrentAdress;
				uint16_t crc = ISO14443_CRCA(Buffer, 2);
				Buffer[2] = crc;
				Buffer[3] = crc >> 8;

				MFURead_CurrentAdress += 4;

				return addParityBits(Buffer, 32);
			}
			return rVal;
		}

		/************************************
		 * This function identifies a PICC. *
		 ************************************/
		case Reader14443_Identify:
		{
			uint16_t rVal = Reader14443A_Select(Buffer, BitCount);
			if (Selected)
			{
				if (ReaderState >= STATE_SAK_CL1 && ReaderState <= STATE_SAK_CL3)
				{
					bool ISO14443_4A_compliant = IS_ISO14443A_4_COMPLIANT(Buffer);
					CardCandidatesIdx = 0;

					uint8_t i;
					for (i = 0; i < ARRAY_COUNT(CardIdentificationList); i++)
					{
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

					if (ISO14443_4A_compliant)
					{
						// send RATS
						return Reader14443A_RATS(Buffer);
					}
					// if we don't have to send the RATS, we are finished for distinguishing with ISO 14443A

				} else if (ReaderState == STATE_ATS) { // we have got the ATS
					BitCount = removeSOC(Buffer, BitCount);
					if (!checkParityBits(Buffer, BitCount))
					{
						LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (BitCount + 8) / 7);
						return Reader14443A_Deselect(Buffer);
					}
					BitCount = removeParityBits(Buffer, BitCount);

					if (Buffer[0] != BitCount / 8 - 2 || ISO14443_CRCA(Buffer, Buffer[0] + 2))
					{
						return Reader14443A_Deselect(Buffer);
					}

					uint8_t i;
					for (i = 0; i < CardCandidatesIdx; i++)
					{
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
							CardCandidates[j] = CardCandidates[j+1];
						CardCandidatesIdx--;
						i--;
					}
				}

				/*
				 * If any cards are not distinguishable with ISO14443A commands only, this is the place to run some proprietary commands.
				 */

				if ((ReaderState >= STATE_SAK_CL1 && ReaderState <= STATE_SAK_CL3) || ReaderState == STATE_ATS)
				{
					uint8_t i;
					for (i = 0; i < CardCandidatesIdx; i++)
					{
						switch (CardCandidates[i])
						{
						case CardType_NXP_MIFARE_DESFire:
						case CardType_NXP_MIFARE_DESFire_EV1:
							Buffer[0] = 0x02;
							Buffer[1] = 0x60;
							uint16_t crc = ISO14443_CRCA(Buffer, 2);
							Buffer[2] = crc;
							Buffer[3] = crc >> 8;
							ReaderState = STATE_DESFIRE_INFO;
							return addParityBits(Buffer, 32);

						default:
							break;
						}
					}
				} else {
					switch (ReaderState)
					{
					case STATE_DESFIRE_INFO:
						if (BitCount == 0)
						{
							CardCandidatesIdx = 0; // this will return that this card is unknown to us
							break;
						}
						BitCount = removeSOC(Buffer, BitCount);
						if (!checkParityBits(Buffer, BitCount))
						{
							LogEntry(LOG_ERR_APP_CHECKSUM_FAIL, Buffer, (BitCount + 8) / 7);
							CardCandidatesIdx = 0;
							return Reader14443A_Deselect(Buffer);
						}
						BitCount = removeParityBits(Buffer, BitCount);
						if (ISO14443_CRCA(Buffer, BitCount / 8))
						{
							CardCandidatesIdx = 0;
							return Reader14443A_Deselect(Buffer);
						}
						switch (Buffer[5])
						{
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

					default:
						break;
					}
				}

				if (CardCandidatesIdx == 0)
				{
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
					for (i = 0; i < CardCandidatesIdx; i++)
					{
						if (size <= TERMINAL_BUFFER_SIZE) // prevents buffer overflow
						{
							char tmpType[64];
							memcpy_P(tmpType, &CardIdentificationList[CardCandidates[i]].Type, 64);
							tmpsize = snprintf(tmpBuf + size, TERMINAL_BUFFER_SIZE - size, "%s or ", tmpType);
							size += tmpsize;
						} else {
							break;
						}
					}
					if (size > TERMINAL_BUFFER_SIZE)
					{
						size -= tmpsize;
						enoughspace = false;
					}
					tmpBuf[size-4] = '.';
					tmpBuf[size-3] = '\0';
					CommandLinePendingTaskFinished(COMMAND_INFO_OK_WITH_TEXT_ID, tmpBuf);
					if (!enoughspace)
						TerminalSendString("There is at least one more card type candidate, but there was not enough terminal buffer space.\r\n");
				}
				// print general data
				TerminalSendString("ATQA:\t");
				CommandLineAppendData(&CardCharacteristics.ATQA, 2);
				TerminalSendString("UID:\t");
				CommandLineAppendData(CardCharacteristics.UID, CardCharacteristics.UIDSize);
				TerminalSendString("SAK:\t");
				CommandLineAppendData(&CardCharacteristics.SAK, 1);

				Reader14443CurrentCommand = Reader14443_Do_Nothing;
				CardCandidatesIdx = 0;
				CodecReaderFieldStop();
				Selected = false;
				return 0;
			}
			return rVal;
		}

		default: // e.g. Do_Nothing
			return 0;
	}
	return 0;
}

uint16_t ISO14443_CRCA(uint8_t * Buffer, uint8_t ByteCount)
{
	uint8_t * DataPtr = Buffer;
	uint16_t crc = 0x6363;
	uint8_t ch;
	while (ByteCount--)
	{
		ch = *DataPtr++ ^ crc;
		ch = ch ^ (ch << 4);
		crc = (crc >> 8) ^ (ch << 8) ^ (ch << 3) ^ (ch >> 4);
	}
	return crc;
}
