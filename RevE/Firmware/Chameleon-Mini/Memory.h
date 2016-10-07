/*
 * Flash.h
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "Common.h"

#define MEMORY_FLASH_USART	USARTD0
#define MEMORY_FLASH_PORT	PORTD
#define MEMORY_FLASH_CS		PIN4_bm
#define MEMORY_FLASH_MOSI	PIN3_bm
#define MEMORY_FLASH_MISO	PIN2_bm
#define MEMORY_FLASH_SCK	PIN1_bm

#define MEMORY_SIZE_PER_SETTING	1024

void MemoryInit(void);
void MemoryReadBlock(void* Buffer, uint16_t Address, uint16_t ByteCount);
void MemoryWriteBlock(const void* Buffer, uint16_t Address, uint16_t ByteCount);
void MemoryClear(void);


void MemoryRecall(void);
void MemoryStore(void);

/* For use with XModem */
bool MemoryUploadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount);
bool MemoryDownloadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount);

#endif /* MEMORY_H_ */
