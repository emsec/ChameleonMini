#include "Detection.h"

#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"
#include "../LED.h"
#include "../Random.h"
#include "../Settings.h"
#include <string.h>

#define MFCLASSIC_1K_ATQA_VALUE     0x0004
#define MFCLASSIC_4K_ATQA_VALUE     0x0002
#define MFCLASSIC_1K_SAK_CL1_VALUE  0x08
#define MFCLASSIC_4K_SAK_CL1_VALUE  0x18

#define MEM_UID_CL1_ADDRESS         0x00
#define MEM_UID_CL1_SIZE            4
#define MEM_UID_BCC1_ADDRESS        0x04
#define MEM_KEY_A_OFFSET            48        /* Bytes */
#define MEM_KEY_B_OFFSET            58        /* Bytes */
#define MEM_KEY_SIZE                6         /* Bytes */
#define MEM_SECTOR_ADDR_MASK        0x3C
#define MEM_BYTES_PER_BLOCK         16        /* Bytes */
#define MEM_VALUE_SIZE              4         /* Bytes */

#define ACK_NAK_FRAME_SIZE          4         /* Bits */
#define ACK_VALUE                   0x0A
#define NAK_INVALID_ARG             0x00
#define NAK_CRC_ERROR               0x01
#define NAK_NOT_AUTHED              0x04
#define NAK_EEPROM_ERROR            0x05
#define NAK_OTHER_ERROR             0x06

#define CMD_AUTH_A                  0x60
#define CMD_AUTH_B                  0x61
#define CMD_AUTH_FRAME_SIZE         2         /* Bytes without CRCA */
#define CMD_AUTH_RB_FRAME_SIZE      4         /* Bytes */
#define CMD_AUTH_AB_FRAME_SIZE      8         /* Bytes */
#define CMD_AUTH_BA_FRAME_SIZE      4         /* Bytes */

static enum {
	STATE_HALT,
	STATE_IDLE,
	STATE_READY,
	STATE_ACTIVE,
	STATE_AUTHING,
	STATE_AUTHED_IDLE,
	STATE_WRITE,
	STATE_INCREMENT,
	STATE_DECREMENT,
	STATE_RESTORE
} State;

static uint16_t CardATQAValue;
static uint8_t CardSAKValue;
uint8_t data_save[16] = {0};
static uint8_t turn_flaga = 0;
static uint8_t turn_flagb = 0;
static uint8_t keyb_flag = 0;

uint16_t MifareDetectionAppProcess(uint8_t* Buffer, uint16_t BitCount)
{
	/* Wakeup and Request may occur in all states */
	if ( (BitCount == 7) &&
	/* precheck of WUP/REQ because ISO14443AWakeUp destroys BitCount */
	(((State != STATE_HALT) && (Buffer[0] == ISO14443A_CMD_REQA)) ||
	(Buffer[0] == ISO14443A_CMD_WUPA) )){

		if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, false)) {
			State = STATE_READY;
			return BitCount;
		}
	}

	switch(State) {
		case STATE_IDLE:
		case STATE_HALT:
			if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, true)) {
				State = STATE_READY;
				return BitCount;
			}		
		break;

		case STATE_READY:
			if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, false)) {
				State = STATE_READY;
				return BitCount;
			} else if (Buffer[0] == ISO14443A_CMD_SELECT_CL1) {
				/* Load UID CL1 and perform anti-collision */
				uint8_t UidCL1[4];
				MemoryReadBlock(UidCL1, MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
				if (ISO14443ASelect(Buffer, &BitCount, UidCL1, CardSAKValue)) {
					State = STATE_ACTIVE;
				}
				return BitCount;
			} else {
				/* Unknown command. Enter HALT state. */
				State = STATE_HALT;
			}	
		break;

		case STATE_ACTIVE:
			if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue, false)) {
				State = STATE_READY;
				return BitCount;
			} else if ( (Buffer[0] == CMD_AUTH_A) || (Buffer[0] == CMD_AUTH_B)) {
				if (ISO14443ACheckCRCA(Buffer, CMD_AUTH_FRAME_SIZE)) {
					uint8_t CardNonce[4] = {0};

					/* Generate a random nonce and read UID and key from memory */
					RandomGetBuffer(CardNonce, sizeof(CardNonce));
				
					memcpy(data_save, Buffer, 4);
					memcpy(data_save+4, CardNonce, 4);

					if (Buffer[0] == CMD_AUTH_B)
					keyb_flag = 1;
					else
					keyb_flag = 0;

					State = STATE_AUTHING;

					for (uint8_t i=0; i<sizeof(CardNonce); i++)
					Buffer[i] = CardNonce[i];

					return CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE;
				}
			}
		break;

		case STATE_AUTHING:
			State = STATE_IDLE;
		
			// Save information
			memcpy(data_save + 8, Buffer, 8);

			if (!keyb_flag) {
				// MEM_OFFSET_DETECTION_DATA
				//#define MEM_OFFSET_DETECTION_DATA  4096 + 16
				// Since there is lots more of space in flashmemory,  it should be able to save many more nonces.
				// also, current implementation overwrites memory space for next slot.
				//
				// KEY A) 0,1,2,3,4,5  *  16 + 4096
				// KEY B) 0,1,2,3,4,5  *  16 + +112 + 4096
				// 5*16 ?
				MemoryWriteBlock(data_save, (turn_flaga * MEM_BYTES_PER_BLOCK) + 4096, MEM_BYTES_PER_BLOCK);
				turn_flaga++;
				turn_flaga = turn_flaga % 6;
			} else {
				MemoryWriteBlock(data_save, (turn_flagb * MEM_BYTES_PER_BLOCK) + 112 + 4096, MEM_BYTES_PER_BLOCK);
				turn_flagb++;
				turn_flagb = turn_flagb % 6;
			}
		break;
		
		default:
		break;
	}

	/* No response has been sent, when we reach here */
	return ISO14443A_APP_NO_RESPONSE;
}

void MifareDetectionInit(void) {
	State = STATE_IDLE;
	CardATQAValue = MFCLASSIC_1K_ATQA_VALUE;
	CardSAKValue = MFCLASSIC_1K_SAK_CL1_VALUE;
}

void MifareDetectionReset(void) {
	State = STATE_IDLE;
}
