#ifndef READER14443A_H
#define READER14443A_H

#include "Application.h"
#include "Codec/Codec.h"

#define CRC_INIT 0x6363

uint8_t ReaderSendBuffer[CODEC_BUFFER_SIZE];
uint16_t ReaderSendBitCount;

void Reader14443AAppInit(void);
void Reader14443AAppReset(void);
void Reader14443AAppTask(void);
void Reader14443AAppTick(void);
void Reader14443AAppTimeout(void);

uint16_t Reader14443AAppProcess(uint8_t* Buffer, uint16_t BitCount);

uint16_t addParityBits(uint8_t * Buffer, uint16_t bits);
uint16_t removeParityBits(uint8_t * Buffer, uint16_t BitCount);
uint16_t removeSOC(uint8_t * Buffer, uint16_t BitCount);
bool checkParityBits(uint8_t * Buffer, uint16_t BitCount);
uint16_t ISO14443_CRCA(uint8_t * Buffer, uint8_t ByteCount);

typedef enum {
	Reader14443_Do_Nothing,
	Reader14443_Send,
	Reader14443_Send_Raw,
	Reader14443_Get_UID,
	Reader14443_Read_MF_Ultralight,
	Reader14443_Identify
} Reader14443Command;


#endif //READER14443A_H
