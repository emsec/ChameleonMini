/*
 * Flash.c
 *
 *  Created on: 20.03.2013
 *      Author: skuser
 */

#include "Memory.h"
#include "Configuration.h"
#include "Common.h"
#include "Settings.h"
#include "LEDHook.h"

/* 4k SRAM buffer */
static uint8_t Memory[MEMORY_SIZE_PER_SETTING];

/* Declarations from assembler file */
uint16_t FlashReadWord(uint32_t Address);
void FlashEraseApplicationPage(uint32_t Address);
void FlashLoadFlashWord(uint16_t Address, uint16_t Data);
void FlashEraseWriteApplicationPage(uint32_t Address);
void FlashEraseFlashBuffer(void);
void FlashWaitForSPM(void);

/* Convert defines from Makefile */
#define FLASH_DATA_START		FLASH_DATA_ADDR
#define FLASH_DATA_END			(FLASH_DATA_ADDR + FLASH_DATA_SIZE - 1)

INLINE void FlashRead(void* Buffer, uint32_t Address, uint16_t ByteCount)
{
	uint8_t* BufPtr = (uint8_t*) Buffer;

	/* We assume that ByteCount is a multiple of 2 */
	uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

	if ( (PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END) ) {
		/* Sanity check to limit access to the allocated area */
		while(ByteCount > 1) {
			uint16_t Word = FlashReadWord(PhysicalAddress);

			*BufPtr++ = (Word >> 0) & 0xFF;
			*BufPtr++ = (Word >> 8) & 0xFF;

			PhysicalAddress += 2;
			ByteCount -= 2;
		}
	}
}

INLINE void FlashWrite(const void* Buffer, uint32_t Address, uint16_t ByteCount)
{
	const uint8_t* BufPtr = (uint8_t*) Buffer;

	/* We assume that FlashWrite is always called for write actions that are
	 * aligned to APP_SECTION_PAGE_SIZE and a multiple of APP_SECTION_PAGE_SIZE.
	 * Thus only full pages are written into the flash. */
	uint16_t PageCount = ByteCount / APP_SECTION_PAGE_SIZE;
	uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

	if ( (PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END) ) {
		/* Sanity check to limit access to the allocated area */

		while(PageCount-- > 0) {
			/* For each page to program, wait for NVM to get ready,
			 * erase the flash page buffer, program all data to the
			 * flash page buffer and write buffer to flash using
			 * the atomic erase and write operation. */

			FlashWaitForSPM();

			FlashEraseFlashBuffer();
			FlashWaitForSPM();

			for (uint16_t i=0; i<APP_SECTION_PAGE_SIZE; i += 2) {
				uint16_t Word = 0;

				Word |= ( (uint16_t) *BufPtr++ << 0);
				Word |= ( (uint16_t) *BufPtr++ << 8);

				FlashLoadFlashWord(i, Word);
				FlashWaitForSPM();
			}

			FlashEraseWriteApplicationPage(PhysicalAddress);
			FlashWaitForSPM();

			PhysicalAddress += APP_SECTION_PAGE_SIZE;
		}
	}
}

INLINE void FlashErase(uint32_t Address, uint16_t ByteCount)
{
	uint16_t PageCount = ByteCount / APP_SECTION_PAGE_SIZE;
	uint32_t PhysicalAddress = Address + FLASH_DATA_ADDR;

	if ( (PhysicalAddress >= FLASH_DATA_START) && (PhysicalAddress <= FLASH_DATA_END) ) {
		/* Sanity check to limit access to the allocated area */
		while(PageCount-- > 0) {
			FlashWaitForSPM();

			FlashEraseApplicationPage(PhysicalAddress);
			FlashWaitForSPM();

			PhysicalAddress += APP_SECTION_PAGE_SIZE;
		}
	}
}

void MemoryInit(void)
{
	/* Load permanent flash contents to memory at startup */
	MemoryRecall();
}

void MemoryReadBlock(void* Buffer, uint16_t Address, uint16_t ByteCount)
{
	uint8_t* SrcPtr = &Memory[Address];
	uint8_t* DstPtr = (uint8_t*) Buffer;

	/* Prevent Buf Ovfs */
	ByteCount = MIN(ByteCount, MEMORY_SIZE_PER_SETTING - Address);

	while(ByteCount--) 
	{
		*DstPtr++ = *SrcPtr++;
	}
}

void MemoryWriteBlock(const void* Buffer, uint16_t Address, uint16_t ByteCount)
{
	uint8_t* SrcPtr = (uint8_t*) Buffer;
	uint8_t* DstPtr = &Memory[Address];

	/* Prevent Buf Ovfs */
	ByteCount = MIN(ByteCount, MEMORY_SIZE_PER_SETTING - Address);

	while(ByteCount--) {
		*DstPtr++ = *SrcPtr++;
	}

	LEDHook(LED_MEMORY_CHANGED, LED_ON);
}

void MemoryClear(void)
{
	for (uint16_t i = 0; i < MEMORY_SIZE_PER_SETTING; i++) 
	{
		Memory[i] = 0;
	}
}

void MemoryRecall(void)
{
	/* Recall memory from permanent internal flash */
	FlashRead(Memory, (uint32_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING, MEMORY_SIZE_PER_SETTING);
}

void MemoryStore(void)
{
	/* Store current memory into permanent internal flash */
	FlashWrite(Memory, (uint32_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING, MEMORY_SIZE_PER_SETTING);

	LEDHook(LED_MEMORY_CHANGED, LED_OFF);
	LEDHook(LED_MEMORY_STORED, LED_PULSE);
}

bool MemoryUploadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount)
{
    if (BlockAddress >= MEMORY_SIZE_PER_SETTING) 
	{
        /* Prevent writing out of bounds by silently ignoring it */
        return true;
    } 
	else 
	{
    	/* Calculate bytes left in memory and start writing */
    	uint32_t BytesLeft = MEMORY_SIZE_PER_SETTING - BlockAddress;
		uint32_t FlashAddress = (uint32_t) BlockAddress + (uint32_t) GlobalSettings.ActiveSettingIdx * MEMORY_SIZE_PER_SETTING;
    	ByteCount = MIN(ByteCount, BytesLeft);

    	/* Store into flash */
		
		/// TODO: Do we want to also upload to Flash or only to RAM (& do store then)
    	// FlashWrite(Buffer, FlashAddress, ByteCount);
		//// FIXME, redirect to internal flash
		
		/* Store to local memory */
    	uint8_t* DstPtr = &Memory[BlockAddress];
    	uint8_t* SrcPtr = (uint8_t*) Buffer;

    	while(ByteCount--) 
		{
    		*DstPtr++ = *SrcPtr++;
    	}

		return true;
    }

}

bool MemoryDownloadBlock(void* Buffer, uint32_t BlockAddress, uint16_t ByteCount)
{
    if (BlockAddress >= MEMORY_SIZE_PER_SETTING) 
	{
        /* There are bytes out of bounds to be read. Notify that we are done. */
        return false;
    } 
	else 
	{
    	/* Calculate bytes left in memory and issue reading */
    	uint32_t BytesLeft = MEMORY_SIZE_PER_SETTING - BlockAddress;
    	//uint32_t FlashAddress = (uint32_t) BlockAddress + (uint32_t) GlobalSettings.ActiveSetting * MEMORY_SIZE_PER_SETTING;
    	ByteCount = MIN(ByteCount, BytesLeft);

    	/* Output local memory contents */
    	uint8_t* DstPtr = (uint8_t*) Buffer;
    	uint8_t* SrcPtr = &Memory[BlockAddress];

    	while(ByteCount--) 
		{
    		*DstPtr++ = *SrcPtr++;
    	}

        return true;
    }
}

